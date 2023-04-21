# https://pypi.org/project/railroad-diagrams/
# https://jakearchibald.github.io/svgomg/

from railroad import *

css_rr = """svg.railroad-diagram {
    background-color:hsl(30,20%,95%);
}
svg.railroad-diagram path {
    stroke-width:3;
    stroke:black;
    fill:rgba(0,0,0,0);
}
svg.railroad-diagram text {
    font:bold 14px monospace;
    text-anchor:middle;
}
svg.railroad-diagram text.label{
    text-anchor:start;
}
svg.railroad-diagram text.comment{
    font:italic 12px monospace;
}
svg.railroad-diagram rect{
    stroke-width:3;
    stroke:black;
    fill:hsl(120,100%,90%);
}
svg.railroad-diagram g.non-terminal rect{
    stroke: black;
    fill: white;
)
svg.railroad-diagram rect.group-box {
    stroke: gray;
    stroke-dasharray: 10 5;
    fill: none;
}"""

d = Diagram(
  Sequence(
    Optional(
      Choice(0,
        Terminal("%protected"),
        Terminal("%private")
      ),"skip"
    ),
    ZeroOrMore(
      Choice(0,
        Terminal("%persistent"),
        Terminal("%read-only")
      ),"","skip"
    ),
    Terminal("object"),
    Choice(0,
      Terminal("<NAME>"),
      Terminal("'<NAME>'"),
      Terminal('"<NAME>"')
    ),
    Optional(
      Sequence(
        Terminal("["),
        Optional(
          Terminal("<NUMBER>"), "skip"
        ),
        Terminal("]")
      )
    ),
    Choice(0,
      Terminal(";"),
      Sequence(
        Terminal("{"),
        ZeroOrMore(
          Choice(0,
            NonTerminal("EXTEND-MIB"),
            NonTerminal("COUNTER"),
            NonTerminal("ACTION"),
            NonTerminal("SUBSCRIPTION"),
            NonTerminal("OBJECT"),
            NonTerminal("SELECT"),
            NonTerminal("PARAMETER"),
            NonTerminal("EVENT"),
            NonTerminal("FUNCTION"),
          )
        ),
        Terminal("}")
      )
    )
  )
)

d.writeStandalone(sys.stdout.write, css=css_rr)
