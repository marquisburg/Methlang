/**
 * Mettle Syntax Extension - Advanced Linter
 *
 * Two diagnostic layers:
 * 1. Regex-based: unsupported operators, invalid literals, etc. (instant)
 * 2. Compiler-backed: run Mettle for real semantic errors (type, scope, etc.)
 *
 * Compiler diagnostics run on open, save, and when the extension activates with a .mettle file open.
 * Regex runs on every edit (debounced). Regex-only passes merge with cached compiler diagnostics
 * (same document version) so they cannot wipe compiler results after save or focus changes.
 */

const vscode = require('vscode');
const path = require('path');
const { execFile } = require('child_process');
const fs = require('fs');
const os = require('os');

/** @type {vscode.DiagnosticCollection} */
let diagnosticCollection;
/** @type {vscode.OutputChannel | null} */
let mettleOutputChannel = null;
/** @type {NodeJS.Timeout | null} */
let debounceTimer = null;
const DEBOUNCE_MS = 150;

/** Cached compiler diagnostics: invalidated when document version changes (any edit). */
/** @type {Map<string, { version: number, diagnostics: vscode.Diagnostic[] }>} */
const compilerDiagnosticsCache = new Map();
/** URIs currently running mettle (regex-only updates skipped to avoid wiping results mid-compile). */
/** @type {Set<string>} */
const compilingUriKeys = new Set();

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
  diagnosticCollection = vscode.languages.createDiagnosticCollection('mettle');
  context.subscriptions.push(diagnosticCollection);

  mettleOutputChannel = vscode.window.createOutputChannel('Mettle');
  context.subscriptions.push(mettleOutputChannel);

  const lintDocument = async (doc, runCompiler = false) => {
    if (!doc || doc.languageId !== 'mettle') return;
    const uriKey = doc.uri.toString();

    if (!runCompiler && compilingUriKeys.has(uriKey)) {
      return;
    }

    const regexDiags = lintRegex(doc);
    let compilerDiags = [];

    if (runCompiler) {
      compilingUriKeys.add(uriKey);
      try {
        compilerDiags = await lintCompiler(doc);
        compilerDiagnosticsCache.set(uriKey, { version: doc.version, diagnostics: compilerDiags });
      } finally {
        compilingUriKeys.delete(uriKey);
      }
    } else {
      const cached = compilerDiagnosticsCache.get(uriKey);
      compilerDiags = cached && cached.version === doc.version ? cached.diagnostics : [];
    }

    const merged = mergeDiagnostics(regexDiags, compilerDiags);
    diagnosticCollection.set(doc.uri, merged);
  };

  const debouncedLint = (doc) => {
    if (debounceTimer) clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => lintDocument(doc, false), DEBOUNCE_MS);
  };

  for (const doc of vscode.workspace.textDocuments) {
    if (doc.languageId === 'mettle') lintDocument(doc, true);
  }

  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument((doc) => {
      if (doc.languageId === 'mettle') lintDocument(doc, true);
    }),
    vscode.workspace.onDidCloseTextDocument((doc) => {
      if (doc.languageId !== 'mettle') return;
      const k = doc.uri.toString();
      compilerDiagnosticsCache.delete(k);
      compilingUriKeys.delete(k);
    }),
    vscode.workspace.onDidChangeTextDocument((e) => {
      if (e.document.languageId === 'mettle') debouncedLint(e.document);
    }),
    vscode.workspace.onDidSaveTextDocument((doc) => {
      if (doc.languageId === 'mettle') lintDocument(doc, true);
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
          diagnostics.push(mkDiag(lineIdx, col, m[0].length, msg, 'Mettle'));
        }
      };

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
    diagnostics.push(mkDiag(stringStartLine, stringStartCol, 1, 'Unterminated string literal. Add closing double quote.', 'Mettle'));
  }

  return diagnostics;
}

function mkDiag(line, col, len, msg, source = 'Mettle') {
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
  const cfg = vscode.workspace.getConfiguration('mettle');
  return {
    compilerEnabled: cfg.get('linter.compilerEnabled', true),
    compilerPath: cfg.get('linter.compilerPath', null),
    stdlibPath: cfg.get('linter.stdlibPath', null),
  };
}

/**
 * Walk up from `startDir` looking for bin/mettle(.exe) so a workspace opened on a
 * subfolder (e.g. only `mettle/`) still finds the repo compiler in `../bin/`.
 * @param {string} startDir
 * @returns {string | null}
 */
function findCompilerInAncestors(startDir) {
  let dir = path.resolve(startDir);
  for (let depth = 0; depth < 16; depth++) {
    const win = path.join(dir, 'bin', 'mettle.exe');
    const unix = path.join(dir, 'bin', 'mettle');
    if (fs.existsSync(win)) return win;
    if (fs.existsSync(unix)) return unix;
    const parent = path.dirname(dir);
    if (parent === dir) break;
    dir = parent;
  }
  return null;
}

/**
 * @param {string} workspaceRoot
 * @param {string} filePath - current .mettle file path (used for ancestor search)
 */
function findCompiler(workspaceRoot, filePath) {
  const cfg = getConfig();
  if (cfg.compilerPath) {
    const p = path.isAbsolute(cfg.compilerPath) ? cfg.compilerPath : path.join(workspaceRoot, cfg.compilerPath);
    if (fs.existsSync(p)) return p;
  }
  const fromAncestors = findCompilerInAncestors(path.dirname(filePath));
  if (fromAncestors) return fromAncestors;
  const candidates = [
    path.join(workspaceRoot, 'bin', 'mettle.exe'),
    path.join(workspaceRoot, 'bin', 'mettle'),
    path.join(workspaceRoot, 'mettle.exe'),
    path.join(workspaceRoot, 'mettle'),
  ];
  for (const c of candidates) {
    if (fs.existsSync(c)) return c;
  }
  return process.platform === 'win32' ? 'mettle.exe' : 'mettle';
}

/**
 * Run Mettle compiler and parse stderr/stdout for errors.
 * Compiler prints either:
 *   error: msg\n  --> path:line:column  (when filename is known), or
 *   error: msg\n  --> line N, column M  (when filename is absent)
 */
async function lintCompiler(document) {
  const cfg = getConfig();
  if (!cfg.compilerEnabled) return [];

  const filePath = document.uri.fsPath;
  if (!filePath || !fs.existsSync(filePath)) return [];

  const workspaceRoot = vscode.workspace.getWorkspaceFolder(document.uri)?.uri?.fsPath || path.dirname(filePath);
  const compiler = findCompiler(workspaceRoot, filePath);
  if (!compiler) return [];

  const tempOut = path.join(os.tmpdir(), `mettle-lint-${Date.now()}.s`);

  const args = [
    '-i', filePath,
    '-o', tempOut,
    '-I', path.dirname(filePath),
    '-I', workspaceRoot,
  ];
  if (cfg.stdlibPath) {
    const stdlib = path.isAbsolute(cfg.stdlibPath)
      ? cfg.stdlibPath
      : path.join(workspaceRoot, cfg.stdlibPath);
    args.push('--stdlib', stdlib);
  }

  return new Promise((resolve) => {
    execFile(compiler, args, {
      timeout: 10000,
      maxBuffer: 1024 * 1024,
      cwd: workspaceRoot,
    }, (err, stdout, stderr) => {
      try { fs.unlinkSync(tempOut); } catch (_) {}

      const output = (stdout || '') + (stderr || '');
      const diagnostics = parseCompilerOutput(output, document, workspaceRoot);

      if (diagnostics.length === 0 && err && /** @type {NodeJS.ErrnoException} */ (err).code === 'ENOENT') {
        mettleOutputChannel?.appendLine(
          `[Mettle] Compiler not found: ${compiler}\n` +
            `Set mettle.linter.compilerPath to your mettle executable, or add it to PATH.\n` +
            `Tried repo bin/ by walking up from: ${path.dirname(filePath)}`
        );
        mettleOutputChannel?.show(true);
      } else if (diagnostics.length === 0 && output.trim().length > 0 && /error:/i.test(output)) {
        mettleOutputChannel?.appendLine(
          `[Mettle] Could not parse compiler output for ${filePath}. First lines:\n${output.slice(0, 800)}`
        );
      }

      resolve(diagnostics);
    });
  });
}

/**
 * Strip ANSI codes and parse compiler diagnostics.
 * @param {string} workspaceRoot - used to resolve relative paths in `path:line:column` locations
 */
function parseCompilerOutput(output, document, workspaceRoot) {
  const diagnostics = [];
  const stripped = output.replace(/\x1b\[[0-9;]*m/g, '');
  const lines = stripped.split(/\r?\n/);

  const docPath = path.normalize(document.uri.fsPath);

  /** @param {string} p */
  function resolveReportedPath(p) {
    const trimmed = p.trim();
    if (path.isAbsolute(trimmed)) return path.normalize(trimmed);
    return path.normalize(path.resolve(workspaceRoot, trimmed));
  }

  for (let i = 0; i < lines.length - 1; i++) {
    const header = lines[i].match(/^(error|warning|note):\s*(.+)$/);
    if (!header) continue;

    const kind = header[1];
    const msg = header[2].trim();
    const locText = lines[i + 1];
    if (!locText || !locText.includes('-->')) continue;

    let line1Based;
    let col1Based;

    const lineColForm = locText.match(/^\s*-->\s*line\s+(\d+),\s*column\s+(\d+)/);
    const pathForm = locText.match(/^\s*-->\s*(.+):(\d+):(\d+)\s*$/);

    if (lineColForm) {
      line1Based = parseInt(lineColForm[1], 10);
      col1Based = parseInt(lineColForm[2], 10);
    } else if (pathForm) {
      const reportedPath = resolveReportedPath(pathForm[1]);
      if (!pathsEqualish(reportedPath, docPath)) continue;
      line1Based = parseInt(pathForm[2], 10);
      col1Based = parseInt(pathForm[3], 10);
    } else {
      continue;
    }

    const severity =
      kind === 'warning'
        ? vscode.DiagnosticSeverity.Warning
        : kind === 'note'
          ? vscode.DiagnosticSeverity.Information
          : vscode.DiagnosticSeverity.Error;

    const line = Math.max(0, line1Based - 1);
    const col = Math.max(0, col1Based - 1);

    const safeLine = document.lineCount > 0 ? Math.min(line, document.lineCount - 1) : 0;
    const lineText = document.lineCount > 0 ? document.lineAt(safeLine).text : '';
    const endCol = Math.min(col + 1, Math.max(lineText.length, 1));

    const d = new vscode.Diagnostic(
      new vscode.Range(safeLine, col, safeLine, endCol),
      msg,
      severity
    );
    d.source = 'Mettle';
    diagnostics.push(d);
  }

  return diagnostics;
}

/** Case-insensitive normalized path compare (Windows-friendly; resolves symlinks when possible). */
function pathsEqualish(a, b) {
  const na = path.normalize(a).toLowerCase();
  const nb = path.normalize(b).toLowerCase();
  if (na === nb) return true;
  try {
    const ra = fs.realpathSync(a);
    const rb = fs.realpathSync(b);
    return path.normalize(ra).toLowerCase() === path.normalize(rb).toLowerCase();
  } catch {
    return false;
  }
}

module.exports = { activate, deactivate };
