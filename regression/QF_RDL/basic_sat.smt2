(set-logic QF_RDL)
(declare-fun b () Real)
(set-info :status sat)
(assert (and (< 0 b) (< b 1)))
(check-sat)
(exit)
