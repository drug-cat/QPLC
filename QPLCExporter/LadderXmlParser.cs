using System;
using System.Collections.Generic;
using System.IO;
using System.Xml.Linq;

namespace QPLCExporter
{
    public class LadderElement
    {
        public string Type { get; set; } = "";
        public string Address { get; set; } = "";
        public string ContactType { get; set; } = "";
        public string Op { get; set; } = "";
        public string Left { get; set; } = "";
        public string Right { get; set; } = "";
        public string Label { get; set; } = "";
        public string JumpType { get; set; } = "";
        public string Dest { get; set; } = "";
        public string Source { get; set; } = "";
        public string Input1 { get; set; } = "";
        public string Input2 { get; set; } = "";
        public string Input3 { get; set; } = "";
        public string Input4 { get; set; } = "";
        public string Preset { get; set; } = "";
    }

    public class LadderRung
    {
        public List<List<LadderElement>> Branches { get; set; } = new List<List<LadderElement>>();
        public LadderElement? Coil { get; set; }
        public LadderElement? Move { get; set; }
        public LadderElement? Jump { get; set; }
        public LadderElement? Label { get; set; }
        public LadderElement? Timer { get; set; }
        public LadderElement? Counter { get; set; }
    }

    public class LadderNetwork
    {
        public string Name { get; set; } = "";
        public List<LadderRung> Rungs { get; set; } = new List<LadderRung>();
    }

    public static class LadderXmlParser
    {
        public static List<LadderNetwork> Parse(string filePath)
        {
            var networks = new List<LadderNetwork>();
            var doc = XDocument.Load(filePath);

            foreach (var networkElem in doc.Descendants("network"))
            {
                var network = new LadderNetwork
                {
                    Name = (string?)networkElem.Attribute("name") ?? ""
                };

                foreach (var rungElem in networkElem.Elements("rung"))
                {
                    var rung = new LadderRung();

                    // Branches
                    foreach (var branchElem in rungElem.Elements("branch"))
                    {
                        var branch = new List<LadderElement>();
                        foreach (var elem in branchElem.Elements())
                        {
                            branch.Add(ParseElement(elem));
                        }
                        rung.Branches.Add(branch);
                    }

                    // Direct elements
                    var coil = rungElem.Element("coil");
                    if (coil != null) rung.Coil = ParseElement(coil);

                    var move = rungElem.Element("move");
                    if (move != null) rung.Move = ParseElement(move);

                    var jump = rungElem.Element("jump");
                    if (jump != null) rung.Jump = ParseElement(jump);

                    var label = rungElem.Element("label");
                    if (label != null) rung.Label = ParseElement(label);

                    var timer = rungElem.Element("timer");
                    if (timer != null) rung.Timer = ParseElement(timer);

                    var counter = rungElem.Element("counter");
                    if (counter != null) rung.Counter = ParseElement(counter);

                    network.Rungs.Add(rung);
                }

                networks.Add(network);
            }

            return networks;
        }

        private static LadderElement ParseElement(XElement elem)
        {
            var element = new LadderElement();

            switch (elem.Name.LocalName)
            {
                case "contact":
                    element.Type = "contact";
                    element.Address = (string?)elem.Attribute("address") ?? "";
                    element.ContactType = (string?)elem.Attribute("type") ?? "NO";
                    element.Op = (string?)elem.Attribute("op") ?? "";
                    element.Left = (string?)elem.Attribute("left") ?? "";
                    element.Right = (string?)elem.Attribute("right") ?? "";
                    break;

                case "coil":
                    element.Type = "coil";
                    element.Address = (string?)elem.Attribute("address") ?? "";
                    element.ContactType = (string?)elem.Attribute("type") ?? "set";
                    break;

                case "move":
                    element.Type = "move";
                    element.Dest = (string?)elem.Attribute("dest") ?? "";
                    element.Source = (string?)elem.Attribute("source") ?? "";
                    break;

                case "jump":
                    element.Type = "jump";
                    element.JumpType = (string?)elem.Attribute("type") ?? "jmp";
                    element.Label = (string?)elem.Attribute("label") ?? "";
                    break;

                case "label":
                    element.Type = "label";
                    element.Label = (string?)elem.Attribute("name") ?? "";
                    break;

                case "timer":
                    element.Type = "timer";
                    element.ContactType = (string?)elem.Attribute("type") ?? "";
                    element.Address = (string?)elem.Attribute("output") ?? "";
                    element.Source = (string?)elem.Attribute("duration") ?? "";
                    break;

                case "counter":
                    element.Type = "counter";
                    element.ContactType = (string?)elem.Attribute("type") ?? "";
                    element.Address = (string?)elem.Attribute("output") ?? "";
                    element.Preset = (string?)elem.Attribute("preset") ?? "";
                    element.Input1 = (string?)elem.Attribute("input") ?? "";
                    element.Input2 = (string?)elem.Attribute("reset") ?? "";
                    element.Input3 = (string?)elem.Attribute("load") ?? "";
                    element.Input4 = (string?)elem.Attribute("up") ?? "";
                    // for count_updown: down may also be present
                    if (string.IsNullOrEmpty(element.Input4))
                        element.Input4 = (string?)elem.Attribute("down") ?? "";
                    break;
            }

            return element;
        }
    }
}