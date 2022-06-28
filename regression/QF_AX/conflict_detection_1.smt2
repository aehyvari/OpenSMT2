(set-logic QF_AX)
(set-info :status sat)
(declare-sort I 0)
(declare-sort E 0)
(declare-fun a () (Array I E))
(declare-fun a2 () (Array I E))
(declare-fun i1 () I)
(declare-fun i3 () I)
(declare-fun i4 () I)
(declare-fun i () I)
(assert (= a (store (store a2 i (select a i4)) i (select (store a2 i1 (select (store a2 i3 (select a2 i4)) i4)) i))))
(assert (not (= a a2)))
(check-sat)
