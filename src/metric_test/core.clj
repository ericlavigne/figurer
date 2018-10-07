(ns metric-test.core
  (:require [clojure.pprint :refer [pprint print-table]]
            [clojure.set :refer [intersection]]
            [clojure.test :refer :all]))

(defn- sum [values]
  (apply + 0.0 values))

(defn- mean [values]
  (/ (sum values)
     (count values)))

(defn- stdev [values]
  (if (< (count values) 2)
    0.0
    (let [total (sum values)
          mean (/ total (count values))
          square-diffs (map #(Math/pow (- mean %) 2) values)]
      (Math/sqrt (/ (sum square-diffs)
                    (dec (count values)))))))

(defn- round [value places]
  (let [scale (Math/pow 10 places)]
    (/ (Math/round
         (* value scale))
       scale)))

(defn- compare-baseline-to-results [baseline results tolerance-stdev]
  (let [metric-keys (keys (first results))]
    (into (sorted-map)
      (zipmap metric-keys
        (map (fn [metric]
               (let [baseline-mean (get-in baseline [metric :mean])
                     baseline-stdev (get-in baseline [metric :stdev])
                     result-values (map #(get % metric) results)
                     result-mean (mean result-values)
                     result-stdev (stdev result-values)
                     absolute-difference (when baseline-mean (- result-mean baseline-mean))
                     stdev-difference (when (and baseline-mean baseline-stdev)
                                        (/ absolute-difference baseline-stdev))
                     exceeds-tolerance (or (not (and baseline-mean baseline-stdev))
                                           (> (Math/abs stdev-difference) tolerance-stdev))]
                 {:baseline-mean (when baseline-mean (round baseline-mean 3))
                  :baseline-stdev (when baseline-stdev (round baseline-stdev 3))
                  :result-mean (round result-mean 3)
                  :result-stdev (round result-stdev 3)
                  :absolute-difference (when absolute-difference (round absolute-difference 3))
                  :stdev-difference (when stdev-difference (round stdev-difference 3))
                  :exceeds-tolerance exceeds-tolerance}))
          metric-keys)))))

(defn- format-failure [comparison]
  (let [metrics (keys comparison)
        new-baseline (into (sorted-map)
                       (zipmap metrics
                         (map (fn [m]
                                (sorted-map
                                  :mean (get-in comparison [m :result-mean])
                                  :stdev (get-in comparison [m :result-stdev])))
                           metrics)))
        rows (map (fn [m]
                    (let [stats (get comparison m)]
                      {"Metric" m
                       "Old" (str (:baseline-mean stats)
                               (when (:baseline-stdev stats)
                                 (str " ± " (:baseline-stdev stats))))
                       "New" (str (:result-mean stats) " ± " (:result-stdev stats))
                       "Change" (str (:absolute-difference stats)
                                  (when (:stdev-difference stats)
                                    (str " (" (:stdev-difference stats) " stdev)")))
                       "Unusual" (when (:exceeds-tolerance stats) "*")}))
               (sort metrics))]
    (with-out-str
      (println "")
      (println "")
      (println "Some metrics changed significantly compared to the baseline.")
      (print-table ["Metric" "Old" "New" "Change" "Unusual"] rows)
      (println "")
      (println "New baseline if these changes are accepted:")
      (println "")
      (pprint new-baseline))))

(defn metric-test
  "Compute testfn, apply metrics to the result, and compare with expected metric values in baseline.
   If results match expectation, use (clojure.test.is true) to indicate that test passed.
   If results don't match expectation, create large custom error message with comparison of all
   metrics and a Clojure-readable data structure representing a new proposed baseline."
  [description testfn
   & {:keys [metrics baseline tolerance-stdev min-trials max-trials]
      :or {baseline {} tolerance-stdev 1.0 min-trials 1 max-trials 10}}]
  (testing description
    (let [metric-keys (sort (keys metrics))
          missing-metrics (not= metric-keys (sort (keys baseline)))]
      (loop [results []]
        (let [test-output (testfn)
              metric-output (zipmap metric-keys
                              (map #((metrics %) test-output)
                                metric-keys))
              results (conj results metric-output)
              comparison (compare-baseline-to-results baseline results tolerance-stdev)
              exceeds-tolerance (some :exceeds-tolerance (vals comparison))
              passing (not (or missing-metrics exceeds-tolerance))]
          (cond
            (< (count results) min-trials)
            (recur results)
            
            passing
            (is true)
            
            (>= (count results) max-trials)
            (testing (format-failure comparison) (is false))
            
            :else
            (recur results)))))))
              
