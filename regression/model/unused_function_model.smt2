(set-logic QF_UF)
(set-option :produce-models true)
(declare-sort I 0)
(declare-sort J 0)
(declare-fun f (I I) Bool)
(declare-fun f1 (I J) J)
(declare-fun g (I I I) I)
(declare-fun a () I)
(check-sat)
(get-model)
(exit)
