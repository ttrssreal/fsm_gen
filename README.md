### fsm_gen
Parse a spec file conforming roughly to the following CFG and emmit C++ source code implementing the specified FSM.

#### EBNF
```
<fsm> ::= <fsm_with_init> <fsm_with_acc> | <fsm_with_acc> <fsm_with_init>

<fsm_with_init> ::= <fsm_with_init> <state_block> | <state_block> <fsm_with_init>
                    | <init_block> <state_block> | <state_block> <init_block>

<fsm_with_acc> ::= <fsm_with_acc> <state_block> | <state_block> <fsm_with_acc>
                    | <acc_block> <state_block> | <state_block> <acc_block>

<string> ::= [a-zA-Z0-9]+

<state_name> ::= <string>
<token> ::= <string>

<init_block> ::= "i{" <state_name> "}"
<acc_block> ::= "a{" <state_name> "}"
<state_block> ::= "s{" <state_contents> "}"

<name_block> ::= "n{" <state_name> "}"
<trans_block> ::= "t{" <inp_block> <dst_block> "}"
<inp_block> ::= "i{" <token> "}"
<dst_block> ::= "d{" <state_name> "}"

<state_contents> ::= <name_block> <state_contents_right> | <trans_block> <state_contents>
<state_contents_right> ::= <trans_block> <state_contents_right> | ""
```
