(ns figurer.car-example-test
  (:require [clojure.test :refer :all]
            [clojure.java.io :as io]
            [figurer.car-example :refer :all]
            [figurer.core :as figurer]
            same))

; Found min and max value by running 10 times.
; Then enforce range twice as large to avoid false positives.
; (map #(figurer/expected-value (figurer/figure sharp-turn-problem {:max-seconds %}))
;   (repeat 10 10.0))

(deftest gentle-turn-test
  (doseq [[seconds min-value max-value] [[0.1 403 489]
                                         [1.0 482 557]
                                         [10.0 557 625]]]
    (let [delta (- max-value min-value)
          min-value (- min-value (/ delta 2.0))
          max-value (+ max-value (/ delta 2.0))
          result (figurer/expected-value
                   (figurer/figure gentle-turn-problem
                     {:max-seconds seconds}))]
      (testing (str "Below expectations on gentle turn with "
                 seconds " seconds")
        (is (> result min-value)))
      (testing (str "Surpass expectations on gentle turn with "
                 seconds " seconds")
        (is (< result max-value))))))

(deftest sharp-turn-test
  (doseq [[seconds min-value max-value] [[0.1 -404 -272]
                                         [1.0 -303 -268]
                                         [10.0 -280 -266]]]
    (let [delta (- max-value min-value)
          min-value (- min-value (/ delta 2.0))
          max-value (+ max-value (/ delta 2.0))
          result (figurer/expected-value
                   (figurer/figure sharp-turn-problem
                     {:max-seconds seconds}))]
      (testing (str "Below expectations on sharp turn with "
                 seconds " seconds")
        (is (> result min-value)))
      (testing (str "Surpass expectations on sharp turn with "
                 seconds " seconds")
        (is (< result max-value))))))

