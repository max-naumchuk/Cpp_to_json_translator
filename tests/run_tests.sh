set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$HERE")"
BIN="${1:-${BINARY:-$ROOT/src/fullparser}}"
[[ -x "$BIN" ]] || { echo "Binary not found: $BIN (build it first, or pass its path)"; exit 1; }

pass=0; fail=0; tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT

for input in "$HERE"/inputs/*.cpp; do
  name="$(basename "$input" .cpp)"
  out="$tmp/out.json"; err="$tmp/err.txt"
  "$BIN" "$input" >"$out" 2>"$err"; code=$?

  if [[ "$name" == err_* ]]; then
    want="$HERE/expected/$name.err"
    if [[ "${UPDATE:-0}" == 1 ]]; then
      sed -E 's/^(Semantic error|Parse error) at line [0-9]+: //' "$err" | head -1 >"$want"
    fi
    [[ -f "$want" ]] || { echo "SKIP  $name (no expected/$name.err)"; continue; }
    if [[ $code -ne 0 && ! -s "$out" ]] && grep -qF -- "$(cat "$want")" "$err"; then
      echo "PASS  $name"; pass=$((pass+1))
    else
      echo "FAIL  $name  (exit=$code)"; echo "  want stderr to contain: $(cat "$want")"
      echo "  got: $(head -2 "$err")"; fail=$((fail+1))
    fi
  else
    want="$HERE/expected/$name.json"
    [[ "${UPDATE:-0}" == 1 && $code -eq 0 ]] && cp "$out" "$want"
    [[ -f "$want" ]] || { echo "SKIP  $name (no expected/$name.json)"; continue; }
    if [[ $code -eq 0 ]] && python3 -c '
import json,sys
sys.exit(0 if json.load(open(sys.argv[1]))==json.load(open(sys.argv[2])) else 1)' "$out" "$want" 2>/dev/null; then
      echo "PASS  $name"; pass=$((pass+1))
    else
      echo "FAIL  $name  (exit=$code)"
      [[ $code -ne 0 ]] && echo "  stderr: $(head -2 "$err")" || diff <(python3 -m json.tool "$want") <(python3 -m json.tool "$out") | head -10
      fail=$((fail+1))
    fi
  fi
done
echo; echo "$pass passed, $fail failed"
[[ $fail -eq 0 ]]
