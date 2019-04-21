(ns figurer.core
  (:require [clojure.data.priority-map :refer [priority-map]]
            [figurer.internal :refer :all]
            [incanter.distributions :refer [draw]]))

(defn define-problem
  "Defines the context for all simulation and optimization.
   Includes null solution because problem and solution are
   the same data type in figurer, differing only in degree
   of refinement. Internally, the problem or solution
   instance is referred to as a context.
  
     policy: function from state to actuation distribution
     value: function from state to number
     predict: function from state and actuation to state
     initial-state: state
     depth: integer indicating how many timesteps to consider
     state: vector of doubles
     actuation: vector of doubles
     actuation distribution: vector of incanter distributions
         (sample from each of these distributions to get one actuation)"
  [{:keys [policy value predict initial-state depth] :as options}]
  (let [initial-node-id 0
        direct-value (value initial-state)
        initial-node {:id initial-node-id
                      :state initial-state
                      :policy (policy initial-state)
                      :goal :maximize
                      :visits 0
                      :direct-value direct-value
                      :value direct-value
                      :next-nodes {}
                      :exploration-priority (priority-map)}]
    (merge options
      {:nodes-so-far 1
       :initial-node-id initial-node-id
       :node-id-to-node {initial-node-id initial-node}})))

(defn figure
  "Perform optimization, returning a more optimized context."
  [context {:keys [max-iterations max-seconds] :as options}]
  (when-not (or max-iterations max-seconds)
    (throw (IllegalArgumentException. "figure requires max-iterations or max-seconds")))
  (let [end-nanoseconds (if max-seconds
                          (+ (System/nanoTime)
                             (* max-seconds (Math/pow 10 9)))
                          nil)]
    (loop [context context
           i 0]
      (if (or (and max-iterations (>= i max-iterations))
              (and end-nanoseconds (>= (System/nanoTime) end-nanoseconds)))
        context
        (recur (figure-once context) (inc i))))))

(defn sample-policy
  "Samples from optimized policy for a given state
   (defaulting to initial state). For maximizing
   policies, the result should be the action that
   is expected to maximize the value. For random
   policies, the initial random distribution will
   be sampled.
   TODO: Use simulation results to refine rather
   than just sampling from the initial policy."
  ([context] (sample-policy context (:initial-state context)))
  ([context state] (mapv draw ((:policy context) state))))

(defn sample-plan
  "Returns a list of states and actuations starting
   from a given state (defaulting to initial state).
   Result states is longer than actuations because
   initial state is included."
  ([context]
   (let [first-node ((:node-id-to-node context) (:initial-node-id context))
         node-path (loop [current first-node
                          previous-states [(:state first-node)]
                          previous-actuations []]
                     (let [next-nodes (map (:node-id-to-node context)
                                        (keys (:next-nodes current)))
                           next-node (if (empty? next-nodes)
                                       nil
                                       (apply max-key :value next-nodes))]
                       (if (nil? next-node)
                         {:states previous-states :actuations previous-actuations}
                         (recur next-node
                           (conj previous-states (:state next-node))
                           (conj previous-actuations
                             (get-in current
                               [:next-nodes (:id next-node) :actuation]))))))]
     (loop [last-state (last (:states node-path))
            previous-states (:states node-path)
            previous-actuations (:actuations node-path)
            remaining (- (:depth context) (count (:actuations node-path)))]
       (if (< remaining 1)
         {:states previous-states :actuations previous-actuations}
         (let [actuation (sample-policy context last-state)
               state ((:predict context) last-state actuation)]
           (recur state (conj previous-states state)
             (conj previous-actuations actuation)
             (dec remaining))))))))

(defn expected-value
  "Estimates the average value that would be
   found by sampling many plans from the
   initial state."
  [context]
  (:value ((:node-id-to-node context)
           (:initial-node-id context))))

