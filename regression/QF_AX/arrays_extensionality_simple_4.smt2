(set-logic QF_AX)
(set-info :status sat)
(declare-sort I 0)
(declare-sort E 0)
(declare-fun a () (Array I E))
(declare-fun e1 () E)
(declare-fun e2 () E)
(declare-fun e () E)
(declare-fun i1 () I)
(declare-fun i () I)
(assert (not (= (store (store a i e) i1 e1) (store (store (store a i1 e1) i e) i1 e2))))
(check-sat)
