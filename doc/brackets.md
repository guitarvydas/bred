# Brackets
  bracket =
    | "(" | ")" | "{" | "}" | "[" | "]" | lt | gt
    |  "❲" |  "❳" |  "\«" | "\»" | "⟨" |  "⟩" | "⟪" | "⟫"
    | "⎨" | "⎬" | "⎡"| "⎤" | "⎣" | "⎦"
    | "“" |  "”"
    
  bracketed =
    | brack<"(", ")"> -- br1
    | brack<"{", "}"> -- br2
    | brack<"[", "]"> -- br3
    | brack<lt, gt> -- br4
    | brack<"❲", "❳"> -- br5
    | brack<"⟨", "⟩"> -- br6
    | brack<"⟪", "⟫"> -- br7
    | brack<"⎨", "⎬"> -- br8
    | brack<"⎡", "⎤"> -- br9
    | brack<"⎣", "⎦"> -- br10
    | brack<"\«", "\»"> -- br11
    | verbatim -- br12

  verbatim = "“" (~"”" any)* "”"
