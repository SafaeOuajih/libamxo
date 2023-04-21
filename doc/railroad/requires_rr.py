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
    Terminal("requires"),
    Choice(0,
      Terminal("'<PATH>'"),
      Terminal('"<PATH>"')
    ),
    Terminal(";")
  )
)

d.writeStandalone(sys.stdout.write, css=css_rr)
