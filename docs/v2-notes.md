# cord v2 notes

- `writeFile` gets a defaulted `bool` arg (true) that writes a header comment at the top of the file, like `# Programmatically written by <caller>`. Caller comes from a defaulted `std::source_location` param (file, line, function).
- `.each()` at schema definition time, to apply a constraint to each direct child element, e.g. `add<std::vector<float>>("volumes").each().range(0.f, 1.f)`.
