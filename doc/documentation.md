# BRED - Text manipulation based on matching brackets

![bred.png](bred.png)

BRED is a tool that helps in re-formatting text.

BRED captures bits of text and allows users to fabricate new text based on the captured text.

BRED is unique in that the patterns that it matches are based on matching pairs of brackets.

BRED's ability to recursively match pairs of brackets makes BRED useful for manipulation of structured and nested text, for example, the source code of modern programming languages that are based on `{ ... }` syntax.

BRED's functionality is restricted to matching pairs of brackets and fabricating new text by simply moving/including match captures. BRED leaves more complicated pattern-matching and replacement operations to existing technologies, such as REGEX-based replacement.

BRED skips over unmatched text and leaves it alone, outputting such unmatched text "as is" (including whitespace).  This makes it possible to create pipelines of very-focussed edits - each step in a pipeline "does only one thing well".  As an example, one could write a preprocessor for a compiled language by prefixing the compiler with a pipeline of macro-like micro-edits and one could create preprocessing steps in a programming IDE.

The fact that BRED matches and replaces patterns in a recursive manner makes it possible to create lisp-like macros for non-lisp, text-based languages.  In fact, BRED-based macros can be more general than lisp macros, since BRED does not require all text to be bracketed, as is required by lisp. In lisp `A (B C D) E` is parsed as three separate phrases, whereas BRED allows prefixes like `A` and suffixes like `E` in a single phrase.  BRED recognizes many more kinds of brackets than are recognized by the lisp reader.

## Simplicity

The syntax and use of BRED is very simple.  

A BRED specification, a .bred file, contains two strings:
1. a pattern to be matched
2. the replacement

Strings in the specification are delimited by the Unicode characters `‛` and `’`.

A wider range of users can use BRED without being faced with learning a wall of complicated details and options. 

# Two Steps of BRED
BRED operates in 2 steps:
1. pattern matching
2. replacement, fabrication of new text.

Pattern matching is based on PEG technology embodied by the FAB tool.   See the section below.

Fabrication is specified using a new SCN syntax - a nano-DSL for formatting match captures.
See the section below.

# Recursive Pattern Matching of Bracketed Items
[recursivematching](recursivematching.md)

# Brackets
[[brackets]]

# String Format for Specifications
[[specificationstrings]]

# Pattern Specification Format
[[patternspecification]]


# Fabrication Specification Format
[[fabricationsyntax]]

# Example
[[messagesynonym]]
# See Also
- REGEX capabilities of languages like JavaScript, Python, etc. and command-line tools such as *awk* and *sed* to complement the use of BRED in forming edit chains. 
- The FAB tool and the `transpile()` library.  For more general pattern matching based on PEG grammars and capture replacement based on an SCN (a nano-DSL).
- The FMT-JS library.  Pattern-matching based on Ohm-JS (PEG-like) and an SCN for fabrication, in library form for JavaScript.
- Ohm-JS - a better PEG, see also Ohm-Editor, which is like a REPL for language syntax design
- PEG - theory of code generation for building pattern-matching code ; ostensibly, PEG looks similar to Language Theory tools and syntaxes like BNF, but, instead of defining *languages*, PEG defines *parser generators*, and, therefore, can match patterns that Language Theory-based parsers cannot match.

# Contributors, Maintainers, Collaborators
Help maintaining this tool would be appreciated.

Experience considered a detriment.


