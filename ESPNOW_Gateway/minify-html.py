import sys
import minify_html

if len(sys.argv) < 2:
    print("Usage: minify-html.py <filename>")
    sys.exit(1)

path = sys.argv[1]
filename = path.split("/")[-1]
with open(path, 'r', encoding="utf-8") as f:
    html = f.read()

minified_html = minify_html.minify(html, minify_css=True, minify_js=True)

with open("data/" + filename, "w", encoding="utf-8") as f:
    f.write(minified_html)

print("Minified HTML saved to data/" + filename)