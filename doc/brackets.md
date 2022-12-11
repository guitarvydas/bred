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
