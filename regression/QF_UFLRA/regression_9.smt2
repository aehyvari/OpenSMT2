(set-logic QF_UFLRA)
(set-info :status sat)
(declare-fun x () Real)
(declare-fun f (Real) Real)
(declare-fun g (Real Real) Real)
(assert (and (not (= x 1)) (= 1.0 (f 0.0)) (< 0 (g 0.0 0.0)) (= (f x) (g 0.0 0.0))))
(check-sat)
