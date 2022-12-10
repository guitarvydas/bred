# Specification Strings
A *string* can contain:
1. literal characters
2. variable names

Literal characters form literal parts of the input and output patterns and variable names are used to signify match captures.

Variable names are identifiers enclosed in brackets `«` and `»`.  The rules for forming identifiers are similar to the rules for forming variable names in most languages, i.e. a letter followed by zero or more alpha-nums (underscores not allowed at this time).

A string begins with the Unicode character `‛` and ends with the Unicode character `’`.

A string must not contain either of the string begin/end characters unless they are escaped. 

Strings can cross multiple lines without the need for resorting to concepts such as long strings and template strings.