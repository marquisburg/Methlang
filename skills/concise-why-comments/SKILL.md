---
name: concise-why-comments
description: "Enforce concise code formatting with an extreme-minimal comment policy: default to no comments unless omission would hide critical intent or constraints. Use when writing or revising code where comments must be rare, high-signal, and rationale-focused for experienced engineers."
---

# Concise Why Comments

## Core Rules

- Keep code compact and readable without ornamental structure.
- Default to no comments; only add one when the code alone cannot safely convey critical intent.
- Do not narrate obvious control flow, assignments, or syntax.
- Assume an experienced technical audience; keep tone direct and professional.

## Commenting Standard

- Treat comments as exceptional, not standard practice.
- Add comments only when omission would likely cause a wrong change, misuse, or subtle bug.
- Prefer one precise sentence over multi-line narration.
- Explain why a choice exists, when it matters, and what would break if changed.
- Remove comments that merely restate identifiers or operations.

## Mini-Doc Standard

- Keep mini-docs brief and purpose-driven, and omit them unless required by project conventions.
- Focus on contract, rationale, edge cases, and operational constraints.
- Omit tutorial framing and beginner walkthrough language.

## Editing Checklist

- Shorten verbose expressions while preserving clarity.
- Rename identifiers when that removes the need for explanatory comments.
- Delete any comment that is not strictly necessary.
- If a comment is necessary, ensure it explains why, not what.
