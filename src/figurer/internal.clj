(ns figurer.internal
  "Internal functions that should not be called from outside the figurer library.
   Functions in this namespace may be changed or removed without notice."
  (:require [incanter.distributions :refer [draw]]))

(defn sum [values]
  (apply + values))

(defn stdev [values]
  (if (< (count values) 2)
    0.0
    (let [total (sum values)
          mean (/ total (count values))
          square-diffs (map #(Math/pow (- mean %) 2) values)]
      (Math/sqrt (/ (sum square-diffs)
                    (dec (count values)))))))

(defn figure-next-node
  "Private: Choose next node (or return nil to create new node).
  
   TODO: Choose promising node (or nil if winner so far is clear)
         rather than just choosing randomly."
  [context node-id]
  (let [current-node (get (:node-id-to-node context) node-id)
        next-node-ids (keys (:next-nodes current-node))
        total-visits (:visits current-node)
        values (map #(get-in context [:node-id-to-node % :value]) next-node-ids)
        value-stdev (stdev values)]
    (if (= 0 (rand-int (inc (count next-node-ids)))) ;(>= (Math/sqrt (* total-visits)) (count next-node-ids))
      nil
      (apply max-key
        (fn [node-id]
          (let [node (get-in context [:node-id-to-node node-id])
                visits (:visits node)
                value (:value node)]
            (+ value
               (* value-stdev
                  (Math/sqrt (/ total-visits
                                (+ 1.0 visits)))))))
        next-node-ids))))

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

