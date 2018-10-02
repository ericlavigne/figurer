(ns figurer.core
  (:require [incanter.distributions :refer [draw]]))

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
                      :next-nodes {}}]
    (merge options
      {:nodes-so-far 1
       :initial-node-id initial-node-id
       :node-id-to-node {initial-node-id initial-node}})))

(defn figure-next-node
  "Private: Choose next node (or return nil to create new node).
  
   TODO: Choose promising node (or nil if winner so far is clear)
         rather than just choosing randomly."
  [context node-id]
  (draw (conj (keys
                (:next-nodes (get (:node-id-to-node context)
                                  node-id)))
          nil)))

(defn figure-rollout
  "Private: Reached end of nodes - follow random policy to determine end result.
   Return average value of remaining states."
  [context node-id remaining-depth]
  (if (< remaining-depth 1) 0.0
    (let [start-node (get (:node-id-to-node context) node-id)
          start-actuation (mapv draw (:policy start-node))
          start-state ((:predict context) (:state start-node) start-actuation)]
      (loop [remaining remaining-depth
             total 0.0
             state start-state]
        (let [new-total (+ total ((:value context) state))]
          (if (<= remaining 1)
            (/ new-total remaining-depth)
            (recur (dec remaining) new-total
              ((:predict context)
               state
               (mapv draw ((:policy context) state))))))))))
  
(defn figure-create-node
  "Private: Create new node following node-id.
   Return modified context and created node ID."
  [context parent-node-id]
  (let [parent (get (:node-id-to-node context) parent-node-id)
        actuation (mapv draw (:policy parent))
        state ((:predict context) (:state parent) actuation)
        policy ((:policy context) state)
        node-id (:nodes-so-far context)
        direct-value ((:value context) state)
        new-node {:id node-id
                  :state state
                  :policy policy
                  :goal :maximize
                  :visits 0
                  :direct-value direct-value
                  :value direct-value
                  :next-nodes {}}
        new-context (assoc-in
                      (update context :nodes-so-far inc)
                      [:node-id-to-node node-id]
                      new-node)
        new-context (assoc-in new-context
                      [:node-id-to-node parent-node-id :next-nodes node-id]
                      {:node-id node-id :actuation actuation})]
    [new-context node-id]))

(defn figure-once
  "Private: Perform one rollout to collect statistics."
  [context]
  (let [visited-node-ids (vec (take-while identity
                                (iterate #(figure-next-node context %)
                                  (:initial-node-id context))))
        [context new-node-id] (if (>= (count visited-node-ids) (:depth context))
                                [context nil]
                                (figure-create-node context (last visited-node-ids)))
        visited-node-ids (if new-node-id
                           (conj visited-node-ids new-node-id)
                           visited-node-ids)
        depth-remaining (- (:depth context) (count visited-node-ids))
        value-after-nodes (figure-rollout context (last visited-node-ids) depth-remaining)
        total-after-nodes (* depth-remaining value-after-nodes)
        context (update-in context [:node-id-to-node (last visited-node-ids)]
                  (fn [node]
                    (merge node
                      {:value (/ (+ (:direct-value node)
                                    total-after-nodes)
                                 (inc depth-remaining))
                       :visits (inc (:visits node))})))
        context (loop [node-ids-to-update (drop 1 (reverse visited-node-ids))
                       context context]
                  (if (empty? node-ids-to-update)
                    context
                    (let [node-id (first node-ids-to-update)
                          node (get (:node-id-to-node context) node-id)
                          children (map (:node-id-to-node context)
                                     (keys (:next-nodes node)))
                          max-child-value (apply max (map :value children))
                          depth-after-node (- (:depth context) (count node-ids-to-update))
                          new-value (/ (+ (:direct-value node)
                                          (* max-child-value depth-after-node))
                                       (inc depth-after-node))
                          new-node (merge node
                                     {:value new-value
                                      :visits (inc (:visits node))})]
                      (recur (rest node-ids-to-update)
                        (assoc-in context [:node-id-to-node node-id] new-node)))))]
    context))

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

