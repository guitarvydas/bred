# Pattern Specification Syntax

A pattern specifcation consists of a *string*, delimited by matching left and right quotes, `‛` and `’` respectively.

A *string* can contain:
1. literal characters
2. variable names

Literal characters form literal parts of the input and output patterns and variable names are used to signify match captures.

Variable names are identifiers enclosed in brackets `«` and `»`.  The rules for forming identifiers are similar to the rules for forming variable names in most languages, i.e. a letter followed by zero or more alpha-nums (underscores not allowed).
