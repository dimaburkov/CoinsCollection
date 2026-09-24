# -*- coding: utf-8 -*-
"""
Minimal Markdown -> .docx converter for CoinsCollection task specs.

Supports: # / ## / ### headings, blank-line separated paragraphs,
"- " bullet lists, GitHub-style pipe tables (with a --- separator row),
inline **bold** and `code`, and a "---" horizontal rule.

Usage:  python build_docx.py [input.md] [output.docx]
Default: CoinsCollection-tasks.md -> CoinsCollection-tasks.docx  (next to this script)
"""
import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def runs_from_text(s):
    """Split a line into (text, bold, mono) runs."""
    out = []
    for tok in re.split(r"(\*\*.+?\*\*|`[^`]+`)", s):
        if not tok:
            continue
        if len(tok) >= 4 and tok.startswith("**") and tok.endswith("**"):
            out.append((tok[2:-2], True, False))
        elif len(tok) >= 2 and tok.startswith("`") and tok.endswith("`"):
            out.append((tok[1:-1], False, True))
        else:
            out.append((tok, False, False))
    return out


def run_xml(text, bold=False, mono=False):
    props = []
    if bold:
        props.append("<w:b/>")
    if mono:
        props.append('<w:rFonts w:ascii="Consolas" w:hAnsi="Consolas" w:cs="Consolas"/>')
    rpr = "<w:rPr>" + "".join(props) + "</w:rPr>" if props else ""
    return '<w:r>%s<w:t xml:space="preserve">%s</w:t></w:r>' % (rpr, esc(text))


def runs_xml(text, force_bold=False):
    parts = "".join(run_xml(t, b or force_bold, m) for (t, b, m) in runs_from_text(text))
    return parts or "<w:r><w:t/></w:r>"


def para_xml(text, style=None):
    ppr = '<w:pPr><w:pStyle w:val="%s"/></w:pPr>' % style if style else ""
    return "<w:p>%s%s</w:p>" % (ppr, runs_xml(text))


def bullet_xml(text):
    ppr = ('<w:pPr><w:pStyle w:val="ListParagraph"/>'
           '<w:numPr><w:ilvl w:val="0"/><w:numId w:val="1"/></w:numPr></w:pPr>')
    return "<w:p>%s%s</w:p>" % (ppr, runs_xml(text))


def hr_xml():
    return ('<w:p><w:pPr><w:pBdr>'
            '<w:bottom w:val="single" w:sz="6" w:space="1" w:color="auto"/>'
            '</w:pBdr></w:pPr></w:p>')


def table_xml(rows):
    ncol = max(len(r) for r in rows)
    grid = "".join("<w:gridCol/>" for _ in range(ncol))
    borders = "<w:tblBorders>" + "".join(
        '<w:%s w:val="single" w:sz="4" w:space="0" w:color="auto"/>' % e
        for e in ("top", "left", "bottom", "right", "insideH", "insideV")
    ) + "</w:tblBorders>"
    tblpr = ('<w:tblPr><w:tblStyle w:val="TableGrid"/>'
             '<w:tblW w:w="0" w:type="auto"/>%s'
             '<w:tblLook w:val="04A0"/></w:tblPr>' % borders)
    body = ""
    for i, r in enumerate(rows):
        header = i == 0
        cells = ""
        for j in range(ncol):
            val = r[j] if j < len(r) else ""
            shd = '<w:shd w:val="clear" w:color="auto" w:fill="F2F2F2"/>' if header else ""
            cells += ('<w:tc><w:tcPr><w:tcW w:w="0" w:type="auto"/>%s</w:tcPr>'
                      '<w:p>%s</w:p></w:tc>' % (shd, runs_xml(val, force_bold=header)))
        trpr = "<w:trPr><w:tblHeader/></w:trPr>" if header else ""
        body += "<w:tr>%s%s</w:tr>" % (trpr, cells)
    return "<w:tbl>%s<w:tblGrid>%s</w:tblGrid>%s</w:tbl><w:p/>" % (tblpr, grid, body)


def parse_blocks(md):
    lines = md.split("\n")
    blocks = []
    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i].rstrip()
        s = raw.strip()
        if not s:
            i += 1
            continue
        if s == "---":
            blocks.append(("hr", None))
            i += 1
            continue
        m = re.match(r"^(#{1,3})\s+(.*)$", s)
        if m:
            blocks.append(("h%d" % len(m.group(1)), m.group(2).strip()))
            i += 1
            continue
        if s.startswith("- "):
            blocks.append(("bullet", s[2:].strip()))
            i += 1
            continue
        if s.startswith("|"):
            tbl = []
            while i < n and lines[i].strip().startswith("|"):
                tbl.append(lines[i].strip())
                i += 1
            rows = []
            for row in tbl:
                cells = [c.strip() for c in row.strip().strip("|").split("|")]
                if cells and all(re.match(r"^:?-{2,}:?$", c) for c in cells):
                    continue
                rows.append(cells)
            if rows:
                blocks.append(("table", rows))
            continue
        # paragraph: gather following plain lines
        buf = [s]
        i += 1
        while i < n:
            nxt = lines[i].rstrip()
            ns = nxt.strip()
            if (not ns or ns == "---" or re.match(r"^#{1,3}\s", ns)
                    or ns.startswith("- ") or ns.startswith("|")):
                break
            buf.append(ns)
            i += 1
        blocks.append(("p", " ".join(buf)))
    return blocks


def blocks_to_body(blocks):
    out = []
    for kind, payload in blocks:
        if kind == "h1":
            out.append(para_xml(payload, "Heading1"))
        elif kind == "h2":
            out.append(para_xml(payload, "Heading2"))
        elif kind == "h3":
            out.append(para_xml(payload, "Heading3"))
        elif kind == "bullet":
            out.append(bullet_xml(payload))
        elif kind == "table":
            out.append(table_xml(payload))
        elif kind == "hr":
            out.append(hr_xml())
        else:
            out.append(para_xml(payload))
    return "".join(out)


CONTENT_TYPES = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
    '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
    '<Default Extension="xml" ContentType="application/xml"/>'
    '<Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>'
    '<Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>'
    '<Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>'
    '</Types>'
)

RELS = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>'
    '</Relationships>'
)

DOC_RELS = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>'
    '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>'
    '</Relationships>'
)

STYLES = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">'
    '<w:docDefaults><w:rPrDefault><w:rPr>'
    '<w:rFonts w:ascii="Calibri" w:hAnsi="Calibri" w:cs="Calibri"/>'
    '<w:sz w:val="22"/><w:szCs w:val="22"/><w:lang w:val="ru-RU"/>'
    '</w:rPr></w:rPrDefault>'
    '<w:pPrDefault><w:pPr><w:spacing w:after="120" w:line="276" w:lineRule="auto"/></w:pPr></w:pPrDefault>'
    '</w:docDefaults>'
    '<w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>'
    '<w:style w:type="paragraph" w:styleId="Heading1"><w:name w:val="heading 1"/><w:basedOn w:val="Normal"/>'
    '<w:next w:val="Normal"/><w:pPr><w:keepNext/><w:spacing w:before="280" w:after="120"/><w:outlineLvl w:val="0"/></w:pPr>'
    '<w:rPr><w:b/><w:sz w:val="34"/><w:szCs w:val="34"/></w:rPr></w:style>'
    '<w:style w:type="paragraph" w:styleId="Heading2"><w:name w:val="heading 2"/><w:basedOn w:val="Normal"/>'
    '<w:next w:val="Normal"/><w:pPr><w:keepNext/><w:spacing w:before="240" w:after="100"/><w:outlineLvl w:val="1"/></w:pPr>'
    '<w:rPr><w:b/><w:sz w:val="28"/><w:szCs w:val="28"/></w:rPr></w:style>'
    '<w:style w:type="paragraph" w:styleId="Heading3"><w:name w:val="heading 3"/><w:basedOn w:val="Normal"/>'
    '<w:next w:val="Normal"/><w:pPr><w:keepNext/><w:spacing w:before="200" w:after="80"/><w:outlineLvl w:val="2"/></w:pPr>'
    '<w:rPr><w:b/><w:sz w:val="24"/><w:szCs w:val="24"/></w:rPr></w:style>'
    '<w:style w:type="paragraph" w:styleId="ListParagraph"><w:name w:val="List Paragraph"/><w:basedOn w:val="Normal"/>'
    '<w:pPr><w:ind w:left="720"/></w:pPr></w:style>'
    '<w:style w:type="table" w:default="1" w:styleId="TableNormal"><w:name w:val="Normal Table"/>'
    '<w:tblPr><w:tblCellMar><w:top w:w="60" w:type="dxa"/><w:left w:w="108" w:type="dxa"/>'
    '<w:bottom w:w="60" w:type="dxa"/><w:right w:w="108" w:type="dxa"/></w:tblCellMar></w:tblPr></w:style>'
    '<w:style w:type="table" w:styleId="TableGrid"><w:name w:val="Table Grid"/><w:basedOn w:val="TableNormal"/>'
    '<w:tblPr><w:tblBorders>'
    '<w:top w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '<w:left w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '<w:bottom w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '<w:right w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '<w:insideH w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '<w:insideV w:val="single" w:sz="4" w:space="0" w:color="auto"/>'
    '</w:tblBorders></w:tblPr></w:style>'
    '</w:styles>'
)

NUMBERING = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">'
    '<w:abstractNum w:abstractNumId="0"><w:multiLevelType w:val="hybridMultilevel"/>'
    '<w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="bullet"/><w:lvlText w:val="&#8226;"/>'
    '<w:lvlJc w:val="left"/><w:pPr><w:ind w:left="720" w:hanging="360"/></w:pPr>'
    '<w:rPr><w:rFonts w:ascii="Symbol" w:hAnsi="Symbol" w:hint="default"/></w:rPr></w:lvl>'
    '</w:abstractNum>'
    '<w:num w:numId="1"><w:abstractNumId w:val="0"/></w:num>'
    '</w:numbering>'
)


def build(md_path, docx_path):
    with open(md_path, "r", encoding="utf-8") as f:
        md = f.read()
    body = blocks_to_body(parse_blocks(md))
    document = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">'
        '<w:body>%s'
        '<w:sectPr><w:pgSz w:w="11906" w:h="16838"/>'
        '<w:pgMar w:top="1134" w:right="1134" w:bottom="1134" w:left="1134" '
        'w:header="709" w:footer="709" w:gutter="0"/></w:sectPr>'
        '</w:body></w:document>' % body
    )
    with zipfile.ZipFile(docx_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", CONTENT_TYPES)
        z.writestr("_rels/.rels", RELS)
        z.writestr("word/document.xml", document)
        z.writestr("word/_rels/document.xml.rels", DOC_RELS)
        z.writestr("word/styles.xml", STYLES)
        z.writestr("word/numbering.xml", NUMBERING)
    print("wrote %s (%d bytes)" % (docx_path, os.path.getsize(docx_path)))


if __name__ == "__main__":
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "CoinsCollection-tasks.md")
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "CoinsCollection-tasks.docx")
    build(src, dst)
