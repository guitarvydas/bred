# Recursive Matching of Bracketed Items

In the pattern matching phase, BRED matches 
- the pattern specified by the BRED pattern matching specification
- bracketed items, and,
- "everything else".

Bracket items begin with one of recognized brackets and end with the corresponding bracket.  

If the bracketed item contains other bracketed items, pattern matching proceeds recursively.  The inner bracketed items are matched recursively using the same matching criteria as specified above.

Inner bracketed items are replaced by their fabrications in a depth-first manner.

Items bracketed by   `“ ... ”` are not parsed recursively and are treated as *verbatim* characters.

See the Brackets section for a list of recognized brackets