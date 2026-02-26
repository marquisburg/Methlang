/**
 * MethASM Syntax Extension - Advanced Linter
 *
 * Two diagnostic layers:
 * 1. Regex-based: unsupported operators, invalid literals, etc. (instant)
 * 2. Compiler-backed: run methasm for real semantic errors (type, scope, etc.)
 *
 * Compiler diagnostics run on save (or configurable). Regex runs on every edit.
 */

const vscode = require('vscode');
const path = require('path');
const { execFile } = require('child_process');
const fs = require('fs');
const os = require('os');

/** @type {vscode.DiagnosticCollection} */
let diagnosticCollection;
/** @type {NodeJS.Timeout | null} */
let debounceTimer = null;
const DEBOUNCE_MS = 150;

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
  diagnosticCollection = vscode.languages.createDiagnosticCollection('masm');
  context.subscriptions.push(diagnosticCollection);

  const lintDocument = async (doc, runCompiler = false) => {
    if (!doc || doc.languageId !== 'masm') return;
    const regexDiags = lintRegex(doc);
    const compilerDiags = runCompiler ? await lintCompiler(doc) : [];
    const merged = mergeDiagnostics(regexDiags, compilerDiags);
    diagnosticCollection.set(doc.uri, merged);
  };

  const debouncedLint = (doc) => {
    if (debounceTimer) clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => lintDocument(doc, false), DEBOUNCE_MS);
  };

  for (const doc of vscode.workspace.textDocuments) {
    if (doc.languageId === 'masm') lintDocument(doc, false);
  }

  context.subscriptions.push(
    vscode.window.onDidChangeActiveTextEditor((editor) => {
      if (editor?.document?.languageId === 'masm') lintDocument(editor.document, false);
    }),
    vscode.workspace.onDidChangeTextDocument((e) => {
      if (e.document.languageId === 'masm') debouncedLint(e.document);
    }),
    vscode.workspace.onDidSaveTextDocument((doc) => {
      if (doc.languageId === 'masm') lintDocument(doc, true);
    })
  );
}

function deactivate() {
  if (debounceTimer) clearTimeout(debounceTimer);
  diagnosticCollection?.dispose();
}

/**
 * Merge regex and compiler diagnostics. Compiler takes precedence on overlapping ranges.
 */
function mergeDiagnostics(regex, compiler) {
  const byKey = new Map();
  for (const d of regex) {
    const k = `${d.range.start.line}:${d.range.start.character}`;
    byKey.set(k, d);
  }
  for (const d of compiler) {
    const k = `${d.range.start.line}:${d.range.start.character}`;
    byKey.set(k, d); // compiler overwrites regex for same position
  }
  return [...byKey.values()];
}

// --- Regex-based linting ---

function getSegments(line, startInString = false) {
  const segments = [];
  let i = 0;
  let segmentStart = 0;
  let inString = startInString;
  let hitComment = false;

  while (i < line.length) {
    if (inString) {
      if (line[i] === '\\') { i += 2; continue; }
      if (line[i] === '"') { inString = false; i++; segmentStart = i; continue; }
      i++;
      continue;
    }
    if (line.indexOf('//', i) === i) {
      if (i > segmentStart) segments.push({ start: segmentStart, end: i, inString: false });
      hitComment = true;
      break;
    }
    if (line[i] === '"') {
      if (i > segmentStart) segments.push({ start: segmentStart, end: i, inString: false });
      inString = true;
      i++;
      segmentStart = i;
      continue;
    }
    i++;
  }
  if (!hitComment && segmentStart < line.length && !inString) {
    segments.push({ start: segmentStart, end: line.length, inString: false });
  }
  return { segments, inString };
}

function lintRegex(document) {
  const diagnostics = [];
  const text = document.getText();
  const lines = text.split(/\r?\n/);

  let inString = false;
  let stringStartLine = 0;
  let stringStartCol = 0;

  for (let lineIdx = 0; lineIdx < lines.length; lineIdx++) {
    const line = lines[lineIdx];
    const { segments, inString: stillInString } = getSegments(line, inString);

    if (!inString && stillInString) {
      const qIdx = line.indexOf('"');
      stringStartLine = lineIdx;
      stringStartCol = qIdx >= 0 ? qIdx : 0;
    }
    inString = stillInString;

    for (const seg of segments) {
      const slice = line.slice(seg.start, seg.end);
      const add = (re, msg) => {
        let m;
        const r = new RegExp(re.source, 'g');
        while ((m = r.exec(slice)) !== null) {
          const col = seg.start + m.index;
          diagnostics.push(mkDiag(lineIdx, col, m[0].length, msg, 'masm'));
        }
      };

      let idx = seg.start;
      while ((idx = line.indexOf('%', idx)) !== -1 && idx < seg.end) {
        diagnostics.push(mkDiag(lineIdx, idx, 1, 'Modulo % is not supported. Use a helper or inline logic.', 'masm'));
        idx++;
      }
      idx = seg.start;
      while ((idx = line.indexOf('~', idx)) !== -1 && idx < seg.end) {
        diagnostics.push(mkDiag(lineIdx, idx, 1, 'Bitwise complement ~ is not supported. Use inline assembly or C externs.', 'masm'));
        idx++;
      }

      add(/<<|>>/g, 'Bitwise shift is not supported. Use inline assembly or C externs.');
      add(/(?<!\|)\|(?!\|)/g, 'Bitwise OR | is not supported. Use && and || for logical operations.');
      add(/\^/g, 'Bitwise XOR ^ is not supported. Use inline assembly or C externs.');
      add(/!(?!=)/g, 'Unary logical NOT ! is not supported. Use == 0 or != 0 for comparisons.');
      add(/[+\-*/]=/g, 'Compound assignment (+=, -=, *=, /=) is not supported. Use x = x + 1 instead.');
      add(/\b0[xX](?=\s|$|[^0-9a-fA-F])/g, 'Invalid hex literal. Expected hex digits after 0x.');
      add(/\b0[bB](?=\s|$|[^01])/g, 'Invalid binary literal. Expected 0 or 1 after 0b.');
      add(/\b[0-9]+\d*_[0-9_]*\b/g, 'Underscores in numeric literals are not supported. Use 1000000 instead of 1_000_000.');
      add(/\/\*/, 'Block comments are not supported. Use // for line comments.');
      add(/\bbreak\s+[a-zA-Z_][a-zA-Z0-9_]*\b/g, 'Labeled break is not supported. Use flags or restructure nested loops.');
      add(/\bcontinue\s+[a-zA-Z_][a-zA-Z0-9_]*\b/g, 'Labeled continue is not supported. Use flags or restructure nested loops.');
    }
  }

  if (inString) {
    diagnostics.push(mkDiag(stringStartLine, stringStartCol, 1, 'Unterminated string literal. Add closing double quote.', 'masm'));
  }

  return diagnostics;
}

function mkDiag(line, col, len, msg, source = 'masm') {
  const d = new vscode.Diagnostic(
    new vscode.Range(line, col, line, col + len),
    msg,
    vscode.DiagnosticSeverity.Error
  );
  d.source = source;
  return d;
}

// --- Compiler-backed linting ---

function getConfig() {
  const cfg = vscode.workspace.getConfiguration('masm');
  return {
    compilerEnabled: cfg.get('linter.compilerEnabled', true),
    compilerPath: cfg.get('linter.compilerPath', null),
    stdlibPath: cfg.get('linter.stdlibPath', null),
  };
}

function findCompiler(workspaceRoot) {
  const cfg = getConfig();
  if (cfg.compilerPath) {
    const p = path.isAbsolute(cfg.compilerPath) ? cfg.compilerPath : path.join(workspaceRoot, cfg.compilerPath);
    return fs.existsSync(p) ? p : null;
  }
  const candidates = [
    path.join(workspaceRoot, 'bin', 'methasm.exe'),
    path.join(workspaceRoot, 'bin', 'methasm'),
    path.join(workspaceRoot, 'methasm.exe'),
    path.join(workspaceRoot, 'methasm'),
  ];
  for (const c of candidates) {
    if (fs.existsSync(c)) return c;
  }
  return null;
}

/**
 * Run methasm compiler and parse stderr/stdout for errors.
 * Compiler prints: "error: msg\n  --> line X, column Y"
 */
async function lintCompiler(document) {
  const cfg = getConfig();
  if (!cfg.compilerEnabled) return [];

  const filePath = document.uri.fsPath;
  if (!filePath || !fs.existsSync(filePath)) return [];

  const workspaceRoot = vscode.workspace.getWorkspaceFolder(document.uri)?.uri?.fsPath || path.dirname(filePath);
  const compiler = findCompiler(workspaceRoot);
  if (!compiler) return [];

  const stdlib = cfg.stdlibPath
    ? (path.isAbsolute(cfg.stdlibPath) ? cfg.stdlibPath : path.join(workspaceRoot, cfg.stdlibPath))
    : path.join(workspaceRoot, 'stdlib');
  const tempOut = path.join(os.tmpdir(), `masm-lint-${Date.now()}.s`);

  const args = [
    '-i', filePath,
    '-o', tempOut,
    '-I', path.dirname(filePath),
    '-I', workspaceRoot,
    '--stdlib', fs.existsSync(stdlib) ? stdlib : path.join(workspaceRoot, 'stdlib'),
  ];

  return new Promise((resolve) => {
    execFile(compiler, args, {
      timeout: 10000,
      maxBuffer: 1024 * 1024,
      cwd: workspaceRoot,
    }, (err, stdout, stderr) => {
      try { fs.unlinkSync(tempOut); } catch (_) {}

      const output = (stdout || '') + (stderr || '');
      const diagnostics = parseCompilerOutput(output, document);

      resolve(diagnostics);
    });
  });
}

/** Strip ANSI codes and parse "error: msg" / "  --> line X, column Y" */
function parseCompilerOutput(output, document) {
  const diagnostics = [];
  const stripped = output.replace(/\x1b\[[0-9;]*m/g, '');

  // Match: "error: message" or "warning: message" followed by "  --> line N, column M"
  const errBlockRe = /(?:error|warning|note):\s*([^\n]+)\n\s*-->\s*line\s+(\d+),\s*column\s+(\d+)/g;
  let m;
  while ((m = errBlockRe.exec(stripped)) !== null) {
    const msg = m[1].trim();
    const line = Math.max(0, parseInt(m[2], 10) - 1);
    const col = Math.max(0, parseInt(m[3], 10) - 1);
    const blockStart = stripped.substring(m.index, m.index + 15);
    const severity = blockStart.includes('warning') ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error;

    const safeLine = document.lineCount > 0 ? Math.min(line, document.lineCount - 1) : 0;
    const lineText = document.lineCount > 0 ? document.lineAt(safeLine).text : '';
    const endCol = Math.min(col + 1, Math.max(lineText.length, 1));

    const d = new vscode.Diagnostic(
      new vscode.Range(safeLine, col, safeLine, endCol),
      msg,
      severity
    );
    d.source = 'methasm';
    diagnostics.push(d);
  }

  return diagnostics;
}

module.exports = { activate, deactivate };
