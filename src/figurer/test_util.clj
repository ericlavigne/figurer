(ns figurer.test-util
  (:require [clojure.test :refer :all]
            [figurer.core :as figurer]))

(defn calculate-baseline [problem seconds]
  (let [solutions (mapv (fn [i] (figurer/figure problem {:max-seconds seconds}))
                    (range 10))
        values (mapv #(figurer/expected-value %) solutions)
        plan-values (mapv (fn [s]
                            (let [plan-states (:states (figurer/sample-plan s))]
                              (/ (apply + (map (:value s)
                                            plan-states))
                                 (count plan-states))))
                      solutions)
        min-value (apply min values)
        max-value (apply max values)
        min-plan-value (apply min plan-values)
        max-plan-value (apply max plan-values)]
    [seconds min-value max-value min-plan-value max-plan-value]))

(defn compare-with-baseline [problem-description problem baseline]
  (doseq [[seconds min-value max-value min-plan-value max-plan-value]
          baseline]
    (let [value-delta (- max-value min-value)
          min-value (- min-value (/ value-delta 2.0))
          max-value (+ max-value (/ value-delta 2.0))
          plan-delta (- max-plan-value min-plan-value)
          min-plan-value (- min-plan-value (/ plan-delta 2.0))
          max-plan-value (+ max-plan-value (/ plan-delta 2.0))
          solution (figurer/figure problem {:max-seconds seconds})
          value (figurer/expected-value solution)
          plan (figurer/sample-plan solution)
          plan-value (/ (apply +
                          (map (:value solution) (:states plan)))
                        (count (:states plan)))]
      (testing (str "Value below expectations on " problem-description " with " seconds " seconds")
        (is (> value min-value)))
      (testing (str "Value surpasses expectations on " problem-description " with " seconds " seconds")
        (is (< value max-value)))
      (testing (str "Plan below expectations on " problem-description " with " seconds " seconds")
        (is (> plan-value min-plan-value)))
      (testing (str "Plan surpasses expectations on " problem-description " with " seconds " seconds")
        (is (< plan-value max-plan-value))))))
