(ns figurer.car-example
  (:require [figurer.core :as figurer]
            [frenet.core :as frenet]
            [incanter.distributions :refer [uniform-distribution]]))

(def steering-pd-parameters
     {:proportional-factor 0.12
      :derivative-factor 1.8})

(def max-speed 85)

(defn pd-actuation
  "Use PD to select actuation (such as steering angle)."
  [{:keys [proportional-error derivative-error] :as pd}
   {:keys [proportional-factor derivative-factor] :as pd-parameters}]
  (- (+ (* proportional-factor proportional-error)
        (* derivative-factor derivative-error))))

(defn pd-steering-estimate
  "Given a state, what steering angle does
   a PD controller recommend?"
  [state]
  (let [[x y psi v vx vy s d vs vd] state]
    (pd-actuation
      {:proportional-error d
       :derivative-error (/ vd
                            (Math/sqrt
                              (+ (* vd vd)
                                 (* vs vs)
                                 0.1)))}
      steering-pd-parameters)))

(defn constrain
  "Return value if between min-value and max-value.
   If below min-value return min-value. If above
   max-value return max-value."
  [value min-value max-value]
  (cond
    (and min-value (< value min-value)) min-value
    (and max-value (> value max-value)) max-value
    :else value))

(defn policy
  "Given current state, determine next actuation.
   Each element of the result vector is a probability
   distribution to represent the uncertainty in which
   actuation would be the best."
  [state]
  (let [[x y psi v vx vy s d vs vd] state
        steering (constrain (pd-steering-estimate state) -0.95 0.95)
        throttle (if (< v max-speed) 0.95 0.05)]
    [(uniform-distribution (- steering 0.05) (+ steering 0.05))
     (uniform-distribution (- throttle 0.05) (+ throttle 0.05))]))

(defn predict
  "Given current state and actuation, determine how
   the state will change."
  [state actuation coord dt]
  (let [[x0 y0 psi0 v0 vx0 vy0 s0 d0 vs0 vd0] state
        [steering throttle] actuation
        Lf 2.67
        steer_radians (* 25 steering (/ Math/PI 180))
        ; Physics
        x (+ x0 (* vx0 dt))
        y (+ y0 (* vy0 dt))
        psi (- psi0 (* v0 dt (/ steer_radians Lf)))
        v (constrain (+ v0 (* throttle dt))
            0.0 (max max-speed v0))
        ; Derived parts of state
        vx (* v (Math/cos psi))
        vy (* v (Math/sin psi))
        [s d vs vd] (frenet/xyv->sdv coord x y vx vy)]
    [x y psi v vx vy s d vs vd]))

(defn value
  "Measure of how 'good' a state is. A plan will
   be chosen that maximizes the average result of
   this function across each state in the plan."
  [state]
  (let [[x y psi v vx vy s d vs vd] state
        progress vs ; Typically 80
        distance-from-center (Math/abs d) ; Range 0-3
        sideways-speed (Math/abs vd) ; Range 0-10
        on-road (< distance-from-center 3.0)]
    (+ (if on-road
         progress ; Credit for speed if on road
         -1000.0) ; Severe penalty if off road
       (* -10.0 distance-from-center) ; Prefer center of road
       (* -1.0 sideways-speed)))) ; Prefer no side-to-side wobbling

(def gentle-turn-initial-state
  [8.248283 0.0 -0.1298784966892089
   82.58185847 81.88632411155638 -10.69547903818482
   13.128100473630248 -0.33590040496936296
   81.76904709786257 12.806004975453702])

(def gentle-turn-waypoints
  [[-4.870652206613999 -0.10922056677204624] [8.576445049756229 -0.3282694602183158]
   [17.398135369535574 0.28799339109713296] [35.53494857092996 4.074226075141546]
   [48.44074084476678 8.106015795237793] [68.24567364411709 16.20987509720734]])

(def gentle-turn-track
  (frenet/track gentle-turn-waypoints))

(def gentle-turn-problem
  (figurer/define-problem {:policy policy
                           :predict (fn [state actuation]
                                      (predict state actuation gentle-turn-track 0.1))
                           :value value
                           :initial-state gentle-turn-initial-state
                           :depth 10}))

(def sharp-turn-initial-state
  [2.101003 0.0 -0.0392492235082796
   100.0579331952381 99.98087338817422 -3.9261879527865218
   2.4520174940088495 -0.5226961044223485
   99.97539967545175 -10.440560664037815])

(def sharp-turn-waypoints
  [[-0.3951281332359218 -0.14911685627552396] [4.475000332980104 -0.8423505836152478]
   [11.099042951712779 -1.8835385902759836] [17.379699272736868 -5.274102256320927]
   [22.694299374648747 -11.153866809188521] [26.71093620998143 -20.241018647528662]])

(def sharp-turn-track
  (frenet/track sharp-turn-waypoints))

(def sharp-turn-problem
  (figurer/define-problem {:policy policy
                           :predict (fn [state actuation]
                                      (predict state actuation sharp-turn-track 0.1))
                           :value value
                           :initial-state sharp-turn-initial-state
                           :depth 10}))

