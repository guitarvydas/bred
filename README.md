Batch Recursive EDitor

Like REGEX, except that it pattern matches patterns enclosed in brackets, recursively.

The running example is mean to convert:

`⟪p val x y⟫`

into 

`⟨MessageWithDebug p val x y⟩`

where any of the args (p, val, x, y) can, recursively, be more `⟪p val x y⟫` patterns.

So, something like

`⟪p ⟪p val x y⟫ v w⟫`

is converted into

`⟨MessageWithDebug ⟨MessageWithDebug p val x y⟩ v w⟩`

See doc/README.md for more detail.


notes to self
  bracket =
    | "(" | ")" | "{" | "}" | "[" | "]" | "<" | ">"
    |  "❲" |  "❳" |  "«" | "»" | "⟨" |  "⟩" | "⟪" | "⟫"

