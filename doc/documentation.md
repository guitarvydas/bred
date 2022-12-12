# BRED - Text manipulation based on matching brackets

![[bred.png]]

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

In the pattern matching phase, BRED matches 
- the pattern specified by the BRED pattern matching specification
- bracketed items, and,
- "everything else".

Bracket items begin with one of recognized brackets and end with the corresponding bracket.  

If the bracketed item contains other bracketed items, pattern matching proceeds recursively.  The inner bracketed items are matched recursively using the same matching criteria as specified above.

Inner bracketed items are replaced by their fabrications in a depth-first manner.

Items bracketed by   `“ ... ”` are not parsed recursively and are treated as *verbatim* characters.

See the Brackets section for a list of recognized brackets/
# Brackets
  bracket =
    | "(" | ")" | "{" | "}" | "[" | "]" | lt | gt
    |  "❲" |  "❳" |  "\«" | "\»" | "⟨" |  "⟩" | "⟪" | "⟫"
    | "⎨" | "⎬" | "⎡"| "⎤" | "⎣" | "⎦"
    | "“" |  "”"

# Matched Brackets
( )
{ }
[ ]
< >
❲ ❳
⟨ ⟩
⟪ ⟫
⎨ ⎬
⎡ ⎤
⎣ ⎦
\« \»

# Verbatim
In the special case that text is bracketed by Unicode quotes `“` ... `”`, the contained text is left alone "as is" and is not recursively matched for deeper bracketing.

# String Format for Specifications

A *string* can contain:
1. literal characters
2. variable names

Literal characters form literal parts of the input and output patterns and variable names are used to signify match captures.

Variable names are identifiers enclosed in brackets `«` and `»`.  The rules for forming identifiers are similar to the rules for forming variable names in most languages, i.e. a letter followed by zero or more alpha-nums (underscores not allowed at this time).

A string begins with the Unicode character `‛` and ends with the Unicode character `’`.

A string must not contain either of the string begin/end characters unless they are escaped. 

Strings can cross multiple lines without the need for resorting to concepts such as long strings and template strings.
# Pattern Specification Syntax

See the section "Specification Strings".

The characters in a pattern are matched *exactly* with no space-skipping.
# Fabrication Specification Syntax

See the section "Specification Strings".

The characters in a fabrication pattern are emitted *exactly*, except when a variable (`« ... »`) is encountered.

If a variable is encountered, the variable is evaluated (tree-walked) and the string returned from the evaluation is inserted "as is" into the fabrication string.

Note that variables are evaluated in a depth-first manner, hence, if a variable evaluation contains another variable, the inner variable is evaluated first (recursively).  The tree-walking evaluator begins at the bottom of the tree and bubbles results upwards.

# Example: A Syntax for Messages

A message ostensibly consists of 2 items
1. a port identifier
2. data

In addition to this, to make debugging messages easier, we add 2 fields
3. come-from ("origin")
4. trail (the message that caused this message (note that this is recursive, the causing message contains a trail, itself, and so on all the way to the beginning)

We would like to use a shorthand syntax for messages when writing and reading code.  On the other hand, a message-sending engine requires more information which can be embodied in a Declarative Type that contains the type-name in more explicit form.

The type-name looks like *noise* to human readers and we would like to elide this kind of detail at the top, human-readable level, while converting (transpiling, editing) it to contain lower-level, machine-readable details.

BRED can perform such a conversion to a block of text.  BRED only rewrites pieces of text that are in the form of human-readable message syntax.  BRED rewrites the human-readable text to its Declarative Type format.

The table below shows the human-readable form and the machine-readable form, a synonym.

| human-readable | synonym |
| --------| ------- |
| ⟪«port» «data» «origin» «trail»⟫ | ⟨Message «port» «data» «origin» «trail»⟩  |

In this syntax, we specify match captures by enclosing names in traditional programming syntax (letter, followed by letters and digits ; no spaces) in Unicode brackets `« ... »`.

We specify the newly fabricated string as a string of characters and named match-capture variables. 

Named match-captures use the same syntax as the pattern-matching syntax, i.e. `«name»`, while everything else is a considered to be a verbatim character.

The BRED specfication for matching and fabricating messages is
```
‛⟪«p» «d» «s» «m»⟫’
‛⟨Message «p» «d» «s» «m»⟩’
```

The first string `‛⟪«p» «d» «s» «m»⟫’` says to find - recursively - all runs of text that
1. begin with a "message" bracket `⟪`
2. have four items separated by single spaces, called *p*, *d*, *s*, and *m* respectively
3. end with a "message end" bracket `⟫`

Each sub-match is scurried away into its corresponding match variable, called *p*, *d*, *s*, and *m* respectively, and made available in the fabrication specification.

The second string `‛⟨Message «p» «d» «s» «m»⟩’` specifies that the newly-fabricated string is to be composed of:
1. a Declarative Type bracket `⟨`
2. The first capture, stored in the variable *p*
3. A single space
4. The second capture, stored in the variable *d*
5. A single space
6. The third capture, stored in the variable *s*
7. A single space
8. The fourth capture, stored in the variable *m*
9. The Declarative Type end bracket `⟩`.

When the pattern is matched, it is replaced by a new string specified by the fabrication spec.

Everything else is untouched and passed-through to the output "as is".

For example, the text
```
	    synonym m ≣ ⟪𝜏dest.port 𝜏dest.message.data š 𝜏dest.message⟫ {
                𝜏dest.target/handle (m 𝜌sendProcedure)
            }

```

matches in only one place, and is output as:

```
	    synonym m ≣ ⟨Message 𝜏dest.port 𝜏dest.message.data š 𝜏dest.message⟩ {
                𝜏dest.target/handle (m 𝜌sendProcedure)
            }

```

(don't worry about the other strange symbols in this text, they have no effect in this very-specific match/fabrication and are left alone "as is")
# Warts
- underscores are not allowed in identifiers, at this time
	- needs to be fixed
	- the grammar(s) needs to be revisited
- escaped quotes
	- in specification strings
	- does this work?
	- if, not, this needs to be fixed
# See Also
- REGEX capabilities of languages like JavaScript, Python, etc. and command-line tools such as *awk* and *sed* to complement the use of BRED in forming edit chains. 
- The FAB tool and the `transpile()` library.  For more general pattern matching based on PEG grammars and capture replacement based on an SCN (a nano-DSL).
- The FMT-JS library.  Pattern-matching based on Ohm-JS (PEG-like) and an SCN for fabrication, in library form for JavaScript.
- Ohm-JS - a better PEG, see also Ohm-Editor, which is like a REPL for language syntax design
- PEG - theory of code generation for building pattern-matching code ; ostensibly, PEG looks similar to Language Theory tools and syntaxes like BNF, but, instead of defining *languages*, PEG defines *parser generators*, and, therefore, can match patterns that Language Theory-based parsers cannot match.

# Contributors, Maintainers, Collaborators
Help maintaining this tool would be appreciated.

Experience considered a detriment.


