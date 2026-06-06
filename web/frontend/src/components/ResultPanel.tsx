import { useState } from 'react'
import type { CompileResult, GrammarData } from '../types'
import OverviewPanel    from './OverviewPanel'
import TokenTable       from './TokenTable'
import AstTree          from './AstTree'
import ParserTrace      from './ParserTrace'
import SymbolTableView  from './SymbolTableView'
import DiagnosticsPanel from './DiagnosticsPanel'
import GrammarView      from './GrammarView'

interface Tab {
  id: string
  label: string
  badge?: number | string
  badgeKind?: 'error' | 'ok' | 'warn'
}

interface Props {
  result: CompileResult
  grammar: GrammarData | null
}

export default function ResultPanel({ result, grammar }: Props) {
  const [tab, setTab] = useState('overview')

  const errCount  = result.error_summary.errors
  const warnCount = result.error_summary.warnings

  const tabs: Tab[] = [
    { id: 'overview',  label: 'Overview' },
    { id: 'tokens',    label: 'Lexer',       badge: Math.max(0, result.tokens.length - 1), badgeKind: 'ok' },
    { id: 'ast',       label: 'AST' },
    { id: 'll1',       label: 'LL(1) Parser' },
    { id: 'lr',        label: 'LR Parser' },
    { id: 'symtab',    label: 'Symbol Table' },
    { id: 'semantics', label: 'Semantics',
      badge: errCount + warnCount || undefined,
      badgeKind: errCount > 0 ? 'error' : warnCount > 0 ? 'warn' : 'ok' },
    { id: 'grammar',   label: 'Grammar' },
  ]

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', overflow: 'hidden' }}>
      {/* Tab bar */}
      <div style={{
        display: 'flex', alignItems: 'center',
        borderBottom: '1px solid var(--border)',
        padding: '0 4px', gap: 0, flexShrink: 0,
        overflowX: 'auto',
      }}>
        {tabs.map(t => (
          <button
            key={t.id}
            onClick={() => setTab(t.id)}
            style={{
              display: 'flex', alignItems: 'center', gap: 5,
              padding: '10px 14px',
              fontSize: 13, fontWeight: tab === t.id ? 600 : 400,
              color: tab === t.id ? 'var(--accent)' : 'var(--text-2)',
              background: 'none', border: 'none',
              borderBottom: tab === t.id ? '2px solid var(--accent)' : '2px solid transparent',
              marginBottom: -1,
              cursor: 'pointer',
              whiteSpace: 'nowrap',
              transition: 'color 0.1s',
              fontFamily: 'var(--font-sans)',
            }}
          >
            {t.label}
            {t.badge !== undefined && (
              <span style={{
                padding: '1px 6px',
                borderRadius: 100,
                fontSize: 10, fontWeight: 600,
                background: t.badgeKind === 'error' ? 'var(--red-bg)'
                           : t.badgeKind === 'warn' ? 'var(--amber-bg)'
                           : 'var(--bg-subtle)',
                color: t.badgeKind === 'error' ? 'var(--red)'
                     : t.badgeKind === 'warn'  ? 'var(--amber)'
                     : 'var(--text-3)',
              }}>
                {t.badge}
              </span>
            )}
          </button>
        ))}
      </div>

      {/* Tab content */}
      <div style={{ flex: 1, overflow: 'auto' }}>
        {tab === 'overview'  && <OverviewPanel result={result} />}
        {tab === 'tokens'    && <TokenTable tokens={result.tokens} />}
        {tab === 'ast'       && <AstTree tree={result.ast_tree} raw={result.ast_text} />}
        {tab === 'll1'       && <ParserTrace kind="ll1" rows={result.ll1_trace} raw={result.ll1_raw} />}
        {tab === 'lr'        && <ParserTrace kind="lr"  rows={result.lr_trace}  raw={result.lr_raw}  />}
        {tab === 'symtab'    && <SymbolTableView scopes={result.symtab} />}
        {tab === 'semantics' && <DiagnosticsPanel diagnostics={result.diagnostics} summary={result.error_summary} />}
        {tab === 'grammar'   && <GrammarView grammar={grammar} />}
      </div>
    </div>
  )
}
