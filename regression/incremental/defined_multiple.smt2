(set-logic QF_UF)

(declare-sort U 0)
(declare-fun x () U)
(declare-fun f (U) U)
(push 1)
(define-fun a1 () Bool (not (= x (f x))))
(assert a1)
(set-info :status sat)
(check-sat)
(pop 1)
(define-fun a1 () Bool false)
(assert a1)
(set-info :status unsat)
(check-sat)
(exit)
