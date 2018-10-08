(ns figurer.car-example-test
  (:require [clojure.test :refer :all]
            [figurer.car-example :refer :all]
            [figurer.core :as figurer]
            [metric-test.core :refer [metric-test]]
            same))

(def metrics
  {:expected-value figurer/expected-value
   :plan-value (fn [solution]
                 (let [plan (figurer/sample-plan solution)
                       plan-value (/ (apply +
                                       (map (:value solution) (:states plan)))
                                     (count (:states plan)))]
                   plan-value))})

(deftest gentle-turn-test
  (metric-test "gentle turn metric 0.1"
    #(figurer/figure gentle-turn-problem {:max-seconds 0.1})
    :metrics metrics
    :baseline
    {:expected-value {:mean 72.508, :stdev 0.542}
     :plan-value {:mean 70.569, :stdev 1.46}})
  (metric-test "gentle turn metric 1.0"
    #(figurer/figure gentle-turn-problem {:max-seconds 1.0})
    :metrics metrics
    :baseline
    {:expected-value {:mean 73.553, :stdev 0.372},
     :plan-value {:mean 70.908, :stdev 2.047}})
  (metric-test "gentle turn metric 10.0"
    #(figurer/figure gentle-turn-problem {:max-seconds 10.0})
    :metrics metrics
    :baseline
    {:expected-value {:mean 73.987, :stdev 0.179},
     :plan-value {:mean 70.722, :stdev 2.352}}))

(deftest sharp-turn-test
  (metric-test "sharp turn metric 0.1"
    #(figurer/figure sharp-turn-problem {:max-seconds 0.1})
    :metrics metrics
    :baseline
    {:expected-value {:mean -339.05, :stdev 40.729},
     :plan-value {:mean -428.533, :stdev 49.654}})
  (metric-test "sharp turn metric 1.0"
    #(figurer/figure sharp-turn-problem {:max-seconds 1.0})
    :metrics metrics
    :baseline
    {:expected-value {:mean -317.563, :stdev 0.909},
     :plan-value {:mean -468.805, :stdev 61.19}})
  (metric-test "sharp turn metric 10.0"
    #(figurer/figure sharp-turn-problem {:max-seconds 10.0})
    :metrics metrics
    :baseline
    {:expected-value {:mean -315.746, :stdev 0.397},
     :plan-value {:mean -477.677, :stdev 56.299}}))

