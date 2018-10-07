(ns figurer.car-example-test
  (:require [clojure.test :refer :all]
            [figurer.car-example :refer :all]
            [figurer.core :as figurer]
            [figurer.test-util :refer :all]
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

(deftest gentle-turn-metric-test
  (metric-test "gentle turn metric 0.1"
    #(figurer/figure gentle-turn-problem {:max-seconds 0.1})
    :metrics metrics
    :baseline {:expected-value {:mean 72.508, :stdev 0.542}
               :plan-value {:mean 70.569, :stdev 1.46}}))

(deftest gentle-turn-test
  (compare-with-baseline "gentle turn" gentle-turn-problem
    [[ 0.1  71.7  73.6  68.6  71.9]
     [ 1.0  73.0  74.1  68.9  72.9]
     [10.0  73.8  74.7  67.2  73.1]]))

(deftest sharp-turn-test
  (compare-with-baseline "sharp turn" sharp-turn-problem
    [[0.1  -418.0  -319.5  -562.6  -391.7]
     [1.0  -319.7  -315.8  -587.9  -397.5]
     [10   -316.9  -313.9  -488.1  -389.8]]))


