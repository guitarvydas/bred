# Example: Syntax for Messages

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