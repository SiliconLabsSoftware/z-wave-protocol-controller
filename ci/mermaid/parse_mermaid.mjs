#!/usr/bin/env node
import { register } from "node:module";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const purifyUrl = pathToFileURL(join(here, "node_modules/dompurify/dist/purify.es.mjs")).href;
const shimUrl = `${pathToFileURL(join(here, "parse_mermaid.mjs")).href}?purify`;
const shimSource = `
import { JSDOM } from "jsdom";
const { default: factory } = await import(${JSON.stringify(purifyUrl)});
const { window } = new JSDOM("<!DOCTYPE html><html><body></body></html>");
export default typeof factory === "function" ? factory(window) : factory;
`;

const hook = `
export async function resolve(specifier, context, nextResolve) {
  if (specifier === "dompurify" && !(context.parentURL || "").includes("?purify")) {
    return { shortCircuit: true, url: ${JSON.stringify(shimUrl)} };
  }
  return nextResolve(specifier, context);
}
export async function load(url, context, nextLoad) {
  if (String(url).includes("parse_mermaid.mjs?purify")) {
    return { format: "module", shortCircuit: true, source: ${JSON.stringify(shimSource)} };
  }
  return nextLoad(url, context);
}
`;

register(`data:text/javascript,${encodeURIComponent(hook)}`, import.meta.url);

const { default: mermaid } = await import("mermaid");

const chunks = [];
for await (const chunk of process.stdin) {
  chunks.push(chunk);
}
const diagrams = JSON.parse(Buffer.concat(chunks).toString("utf8"));

mermaid.initialize({ startOnLoad: false, securityLevel: "strict" });

let failed = 0;
for (const diagram of diagrams) {
  try {
    await mermaid.parse(diagram.source);
  } catch (error) {
    failed += 1;
    const message = error?.str ?? error?.message ?? String(error);
    console.error(`${diagram.file}:${diagram.line}: mermaid parse error: ${message}`);
  }
}

if (failed) {
  console.error(`mermaid.parse failed for ${failed} diagram(s)`);
  process.exit(1);
}
