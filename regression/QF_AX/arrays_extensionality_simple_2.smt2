(set-logic QF_AX)
(set-info :status unsat)
(declare-sort I 0)
(declare-sort E 0)
(declare-fun a () (Array I E))
(declare-fun i () I)
(declare-fun i1 () I)
(assert (not (= (store (store a i (select a i1)) i1 (select a i)) (store (store a i1 (select a i)) i (select a i1)))))
(check-sat)
