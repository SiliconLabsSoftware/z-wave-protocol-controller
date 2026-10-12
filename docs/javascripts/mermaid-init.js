// Re-run Mermaid after Material instant navigation swaps page content.
// Without this, diagrams render only on the first page load.
document$.subscribe(function () {
  mermaid.run({ querySelector: ".mermaid" });
});
