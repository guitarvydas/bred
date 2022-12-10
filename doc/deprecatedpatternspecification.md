# Pattern Specification Syntax

A pattern contains two *strings*, delimited by matching left and right quotes, `‛` and `’` respectively.

The *bred* tool generates code - a script - that converts the first string into the second string.  

The generated code consists of some (3 at the moment of writing) generated files and some boilerplate code.  The first generated file is an *Ohm-JS* compatible grammar and the second generated file is a *Fab* compatible code fabrication script.  The generated code is combined with some boilerplate code and the result is run by the *fab* tool to convert the given input text file.

Parts of the file that don't match the first string are untouched.  The editor matches and replaces only the matching text in the input file.

A *string* can contain:
1. literal characters
2. variable names

Literal characters form literal parts of the input and output patterns and variable names are used to signify match captures.

Variable names are identifiers enclosed in brackets `«` and `»`.  The rules for forming identifiers are similar to the rules for forming variable names in most languages, i.e. a letter followed by zero or more alpha-nums (underscores not allowed).

For example, a *bred* pattern that converts "message" syntax into "declarative type" syntax is:

```
‛⟪«p» «d» «s» «m»⟫’
‛⟨MessageWithDebug «p» «d» «s» «m»⟩’
```
The first string `⟪«p» «d» «s» «m»⟫` is replaced by `⟨MessageWithDebug «p» «d» «s» «m»⟩` where `«p»`, `«d»`, `«s»`, and `«m»` are variables that capture matching text.

When the input text is:
`⟪abc def ghi jkl⟫`
the output is:
`⟨MessageWithDebug abc def ghi jkl⟩`
During the conversion, the variable `«p»` holds the text `abc`, the variable `«d»` holds the string `def`, the variable `«s»`  holds the string `ghi`, and the variable `«m»` holds the string `jkl`.