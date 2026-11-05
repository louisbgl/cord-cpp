# cord v2 grammar

Defines what cord parses. Notation: EBNF (`[ ]` optional, `{ }` zero or more, `|` choice,
`'a'..'z'` range, `-` exception, `(* *)` comment). Lowercase rule names are syntax rules,
UPPERCASE names are tokens produced by the lexer.

## 0. Overview

A cord config is a list of `key = value` entries, one per line. Keys are flat: there are no
sections or dotted paths. Values can be scalars, arrays or objects, and arrays and objects can
nest freely. Whether a value has the right type is decided by the schema, not the grammar.

```
# comments run to end of line
name    = "cord"            # string, double quotes only
port    = 8080              # int
ratio   = -0.75             # float, needs digits on both sides of the dot
verbose = false             # bool, lowercase only
empty   = []                # empty array

tags = ["fast", "small",]   # trailing comma allowed
grid = [[1, 2], [3, 4]]     # arrays nest

player = { name = "bob", pos = { x = 1.5, y = -2.0 } }

# arrays and objects can span lines; commas still separate elements
entities = [
    { name = "a", hp = 10 },
    { name = "b", hp = 20 },
]
```

Rejected:

```
port 8080                   # missing '='
fullscreen                  # bare key (bool sugar not supported yet)
a = 1  b = 2                # two entries on one line
x = .5                      # no leading/trailing dot floats
x = 1e5                     # no exponents
x = 0xFF                    # no hex
flag = True                 # bool is lowercase only
true = 1                    # true/false are reserved, not keys
name = bob                  # strings must be quoted
s = "bad \q"                # unknown escape
n = 123abc                  # numbers must be separated
list = [1 2]                # elements need commas
list = [1, 2                # unclosed bracket
db.host = "x"               # no dots in keys
```

## 1. Lexical grammar

```ebnf
(* ---- characters ---- *)
letter   = 'a'..'z' | 'A'..'Z' ;
digit    = '0'..'9' ;

(* ---- tokens ---- *)
KEY      = ( letter | "_" ) { letter | digit | "_" | "-" } ;   (* excluding reserved words *)
BOOL     = "true" | "false" ;
INT      = [ "-" ] digit { digit } ;
FLOAT    = [ "-" ] digit { digit } "." digit { digit } ;
STRING   = '"' { strchar | escape } '"' ;
strchar  = ? any byte ? - '"' - '\' - NEWLINE ;
escape   = '\' ( '"' | '\' | 'n' | 't' | 'r' ) ;

EQUALS   = "=" ;
COMMA    = "," ;
LBRACKET = "[" ;
RBRACKET = "]" ;
LBRACE   = "{" ;
RBRACE   = "}" ;
NEWLINE  = "\n" | "\r\n" ;
EOF ;

(* ---- skipped, never emitted ---- *)
whitespace = " " | "\t" ;
COMMENT    = "#" { ? any byte ? - NEWLINE } ;
```

### Lexical rules

1. Whitespace (space, tab) between tokens is optional; any amount is skipped.
2. `NEWLINE` is not whitespace: it is a token, significant to the syntax grammar.
3. Maximal munch: the longest match wins. A number directly followed by a letter, `_` or `.`
   is an error (`123abc`, `1.2.3`, `5.`).
4. `true` and `false` are reserved: they always lex as `BOOL`, never as `KEY`.
5. Keys are case-sensitive. `BOOL` is lowercase only.
6. Comments run from `#` to end of line and are always enabled. A `#` inside a `STRING` is
   part of the string.
7. String content is raw bytes: UTF-8 passes through unvalidated. Only the escapes listed in
   `escape` are recognized; any other `\x` is an error.
8. A UTF-8 byte order mark (`EF BB BF`) at the very start of the content is skipped. Anywhere
   else outside a string or comment, it is an error.

## 2. Syntax grammar

```ebnf
content = { [ entry ] NEWLINE } [ entry ] EOF ;
entry   = KEY "=" value ;
value   = BOOL | INT | FLOAT | STRING | array | object ;
array   = "[" [ value { "," value } [ "," ] ] "]" ;
object  = "{" [ entry { "," entry } [ "," ] ] "}" ;
```

The grammar is LL(1): one token of lookahead always decides which rule applies.

### Syntax rules

1. Each line holds at most one entry. Blank and comment-only lines are allowed. The last
   entry may end at `EOF` without a trailing newline.
2. Inside `[ ]` and `{ }`, `NEWLINE` is ignored: the lexer tracks bracket depth and drops
   newlines while depth > 0. An unclosed bracket is reported at its opening position.
3. Elements of arrays and objects are separated by commas only. One trailing comma is allowed.
4. The grammar only checks shape. Types are checked against the schema at binding: nested
   arrays, mixed-type arrays or unexpected objects parse fine and fail only if the schema
   declares something else.

## 3. Semantic rules

Applied after parsing, when the parsed content is bound to a schema.

### Severity

- **Error**: the config is invalid and no result is produced.
- **Warning**: the result is usable; the warning is reported alongside it.

Problems are collected, not thrown one at a time. Recovery is best effort:

1. Syntax error: report it, skip to the next `NEWLINE` at bracket depth 0, resume parsing.
2. If the very next entry also fails to parse, stop: report what was collected so far.
3. If any syntax error occurred, binding is skipped. Only syntax errors are reported, to avoid
   follow-up noise (e.g. a "missing required key" for a key whose line failed to parse).
4. An unclosed bracket swallows the rest of the content (newlines are ignored inside it): one
   error at the opening bracket, then stop.

### Presence

1. Keys are optional by default. An absent optional key with a default is silent.
2. An absent optional key without a default is a warning.
3. A key marked `required` that is absent is an error.

### Constraints

A value that violates a constraint declared in the schema (`min`, `max`, `oneOf`) is an error.

### Keys

1. Duplicate key: the last occurrence wins. Warning. Same rule for keys inside an object.
2. Unknown key (not in the schema): skipped. Warning.

### Types

1. The value's lexical type must match the schema type. A mismatch is an error.
   - `port = "8080"` for an int field: error.
   - `port = 8080.0` for an int field: error.
2. Exception: an `INT` is accepted where a float or double is expected (`ratio = 1`). No warning.
   Precision loss from large ints is silent.
3. An array's elements must all match the declared element type. Each mismatching element is
   its own error, reported with its index (`tags[2]`).
4. An object must match the declared struct: its keys follow the same key rules as top-level
   entries (duplicates and unknown keys warn), its values the same type rules.
5. A number that does not fit the declared type is an error (`port = 99999999999` for a 32-bit
   int field). Float literals too large for the type (out of range, not precision loss) are an
   error too.
6. NaN and infinity are not representable: there is no literal for them, and writing a struct
   holding one is an error.
