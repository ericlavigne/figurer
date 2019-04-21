(ns figurer.car-example-test
  (:require [clojure.test :refer :all]
            [figurer.car-example :refer :all]
            [figurer.core :as figurer]
            [metric-test.core :refer [metric-test]]
            same))

(defn distance [vec1 vec2]
  (Math/sqrt
    (apply +
      (map (fn [x1 x2] (Math/pow (- x1 x2) 2))
        vec1 vec2))))

(defn metrics [best]
  {;:evaluation-depth-max ...
   :evaluation-depth-chosen (fn [solution]
                              (let [plan1 (figurer/sample-plan solution)
                                    plan2 (figurer/sample-plan solution)
                                    which-nodes-match (map = (:states plan1) (:states plan2))]
                                (count (filter identity which-nodes-match))))
   :first-move-candidate-count (fn [solution]
                                 (let [first-node (get-in solution
                                                    [:node-id-to-node
                                                     (:initial-node-id solution)])]
                                   (count (:next-nodes first-node))))
   :first-move-candidate-optimality (fn [solution]
                                      (let [best-first (first (:actuations (figurer/sample-plan best)))
                                            first-node (get-in solution
                                                         [:node-id-to-node
                                                          (:initial-node-id solution)])
                                            all-first-actuations (map :actuation
                                                                   (vals (:next-nodes first-node)))]
                                        (apply min
                                          (map #(distance best-first %)
                                            all-first-actuations))))
   :first-move-chosen-optimality (fn [solution]
                                   (let [solution-first (first (:actuations (figurer/sample-plan solution)))
                                         best-first (first (:actuations (figurer/sample-plan best)))]
                                     (distance solution-first best-first)))
   :iterations (fn [solution]
                 (get-in solution [:node-id-to-node (:initial-node-id solution) :visits]))
   :value-estimate figurer/expected-value
   :value-plan (fn [solution]
                 (let [plan (figurer/sample-plan solution)
                       plan-value (/ (apply +
                                       (map (:value solution) (:states plan)))
                                     (count (:states plan)))]
                   plan-value))})

(defn testfn [problem seconds]
  (fn []
    (Thread/sleep (* seconds 1000))
    (let [solution (figurer/figure problem {:max-seconds seconds})]
      (Thread/sleep (* seconds 1000))
      solution)))

(deftest gentle-turn-test
  (let [best-solution (figurer/figure gentle-turn-problem {:max-seconds 30.0})]
    (metric-test "gentle turn metric 0.1"
      (testfn gentle-turn-problem 0.1)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 3.5, :stdev 0.85},
       :first-move-candidate-count {:mean 16.0, :stdev 2.108},
       :first-move-candidate-optimality {:mean 0.01, :stdev 0.015},
       :first-move-chosen-optimality {:mean 0.033, :stdev 0.04},
       :iterations {:mean 132.3, :stdev 20},
       :value-estimate {:mean 72.399, :stdev 0.626},
       :value-plan {:mean 70.357, :stdev 1.292}})
    (metric-test "gentle turn metric 1.0"
      (testfn gentle-turn-problem 1.0)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 4.7, :stdev 0.675},
       :first-move-candidate-count {:mean 49.3, :stdev 4.923},
       :first-move-candidate-optimality {:mean 0.006, :stdev 0.005},
       :first-move-chosen-optimality {:mean 0.047, :stdev 0.028},
       :iterations {:mean 1364.4, :stdev 100},
       :value-estimate {:mean 73.536, :stdev 0.268},
       :value-plan {:mean 70.803, :stdev 1.073}})
    (metric-test "gentle turn metric 10.0"
      (testfn gentle-turn-problem 10.0)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 5.2, :stdev 0.789},
       :first-move-candidate-count {:mean 164.9, :stdev 6},
       :first-move-candidate-optimality {:mean 0.004, :stdev 0.003},
       :first-move-chosen-optimality {:mean 0.031, :stdev 0.04},
       :iterations {:mean 13504.0, :stdev 1000},
       :value-estimate {:mean 74.092, :stdev 0.278},
       :value-plan {:mean 72.062, :stdev 1.435}})))

(deftest sharp-turn-test
  (let [best-solution (figurer/figure sharp-turn-problem {:max-seconds 30.0})]
    (metric-test "sharp turn metric 0.1"
      (testfn sharp-turn-problem 0.1)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 3.8, :stdev 1.033},
       :first-move-candidate-count {:mean 12.7, :stdev 1.337},
       :first-move-candidate-optimality {:mean 0.007, :stdev 0.015},
       :first-move-chosen-optimality {:mean 0.038, :stdev 0.015},
       :iterations {:mean 97.0, :stdev 20},
       :value-estimate {:mean -329.678, :stdev 30.4},
       :value-plan {:mean -486.6, :stdev 40.203}})
    (metric-test "sharp turn metric 1.0"
      (testfn sharp-turn-problem 1.0)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 4.9, :stdev 1.5},
       :first-move-candidate-count {:mean 45.6, :stdev 5},
       :first-move-candidate-optimality {:mean 0.007, :stdev 0.004},
       :first-move-chosen-optimality {:mean 0.028, :stdev 0.04},
       :iterations {:mean 982.7, :stdev 70},
       :value-estimate {:mean -317.306, :stdev 0.841},
       :value-plan {:mean -455.826, :stdev 45.553}})
    (metric-test "sharp turn metric 10.0"
      (testfn sharp-turn-problem 10.0)
      :metrics (metrics best-solution)
      :baseline
      {:evaluation-depth-chosen {:mean 4.8, :stdev 0.919},
       :first-move-candidate-count {:mean 139.6, :stdev 10},
       :first-move-candidate-optimality {:mean 0.004, :stdev 0.003},
       :first-move-chosen-optimality {:mean 0.031, :stdev 0.012},
       :iterations {:mean 9829.1, :stdev 700},
       :value-estimate {:mean -315.67, :stdev 0.767},
       :value-plan {:mean -457.114, :stdev 89.632}})))

