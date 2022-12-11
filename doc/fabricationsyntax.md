# Fabrication Specification Syntax

See the section "Specification Strings".

The characters in a fabrication pattern are emitted *exactly*, except when a variable (`« ... »`) is encountered.

If a variable is encountered, the variable is evaluated (tree-walked) and the string returned from the evaluation is inserted "as is" into the fabrication string.

Note that variables are evaluated in a depth-first manner, hence, if a variable evaluation contains another variable, the inner variable is evaluated first (recursively).  The tree-walking evaluator begins at the bottom of the tree and bubbles results upwards.