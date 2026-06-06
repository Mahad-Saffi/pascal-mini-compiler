import { useState, useEffect } from 'react'
import type { GrammarData, FirstFollowEntry } from '../types'
import { fetchGrammar } from '../api'

interface Props { grammar: GrammarData | null }

type GTab = 'grammar' | 'first_follow' | 'll1_table' | 'slr_table'

function SetPills({ items }: { items: string[] }) {
  return (
    <div style={{ display: 'flex', gap: 4, flexWrap: 'wrap' }}>
      {items.length === 0
        ? <span style={{ color: 'var(--text-4)', fontSize: 12 }}>{ '∅' }</span>
        : items.map(item => (
          <span key={item} style={{
            padding: '1px 7px', borderRadius: 100, fontSize: 11,
            background: item === 'ε' || item === '$' ? 'var(--bg-subtle)' : 'var(--accent-bg)',
            color:      item === 'ε' || item === '$' ? 'var(--text-3)'   : 'var(--accent-text)',
            border:     '1px solid var(--border)',
            fontFamily: 'var(--font-mono)', fontWeight: 500,
          }}>{item}</span>
        ))
      }
    </div>
  )
}

function FirstFollowTable({ entries }: { entries: FirstFollowEntry[] }) {
  return (
    <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
      <div style={{ overflowX: 'auto', maxHeight: 'calc(100vh - 280px)', overflowY: 'auto' }}>
        <table className="data-table">
          <thead>
            <tr>
              <th>Non-terminal</th>
              <th>FIRST set</th>
              <th>FOLLOW set</th>
            </tr>
          </thead>
          <tbody>
            {entries.map(e => (
              <tr key={e.nonterminal}>
                <td className="mono" style={{ fontWeight: 600, fontSize: 12, whiteSpace: 'nowrap' }}>
                  {e.nonterminal}
                </td>
                <td><SetPills items={e.first} /></td>
                <td><SetPills items={e.follow} /></td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

const GTAB_LABELS: Record<GTab, string> = {
  grammar:      'Original + Transformed',
  first_follow: 'FIRST / FOLLOW',
  ll1_table:    'LL(1) Table',
  slr_table:    'SLR(1) Table',
}

export default function GrammarView({ grammar: initialGrammar }: Props) {
  const [grammar, setGrammar] = useState<GrammarData | null>(initialGrammar)
  const [tab, setTab] = useState<GTab>('grammar')
  const [loading, setLoading] = useState(false)

  useEffect(() => { setGrammar(initialGrammar) }, [initialGrammar])

  const load = async () => {
    if (grammar) return
    setLoading(true)
    try {
      const data = await fetchGrammar()
      setGrammar(data)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => { load() }, [])

  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 12 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10, flexWrap: 'wrap' }}>
        <div className="section-label" style={{ margin: 0 }}>Grammar &amp; Parse Tables</div>
        <span style={{ fontSize: 12, color: 'var(--text-4)' }}>
          Subset of Pascal BNF · FIRST/FOLLOW · LL(1) · SLR(1) ACTION/GOTO
        </span>
      </div>

      {/* Sub-tab bar */}
      <div style={{ display: 'flex', gap: 4, borderBottom: '1px solid var(--border)', paddingBottom: 0 }}>
        {(Object.keys(GTAB_LABELS) as GTab[]).map(t => (
          <button key={t} onClick={() => { setTab(t); load() }} style={{
            padding: '6px 12px', fontSize: 12, fontWeight: tab === t ? 600 : 400,
            color: tab === t ? 'var(--accent)' : 'var(--text-2)',
            background: 'none', border: 'none',
            borderBottom: tab === t ? '2px solid var(--accent)' : '2px solid transparent',
            marginBottom: -1, cursor: 'pointer',
            fontFamily: 'var(--font-sans)', whiteSpace: 'nowrap',
          }}>
            {GTAB_LABELS[t]}
          </button>
        ))}
      </div>

      {loading && (
        <div style={{ padding: 24, color: 'var(--text-4)', textAlign: 'center', fontSize: 13 }}>
          Loading grammar tables…
        </div>
      )}

      {!loading && !grammar && (
        <div className="empty-state"><p>Grammar not loaded</p></div>
      )}

      {!loading && grammar && (
        <>
          {tab === 'grammar' && (
            <pre className="code-block" style={{ maxHeight: 'calc(100vh - 280px)' }}>
              {grammar.grammar_raw}
            </pre>
          )}

          {tab === 'first_follow' && (
            <FirstFollowTable entries={grammar.first_follow} />
          )}

          {tab === 'll1_table' && (
            <pre className="code-block" style={{ maxHeight: 'calc(100vh - 280px)' }}>
              {grammar.ll1_table_raw}
            </pre>
          )}

          {tab === 'slr_table' && (
            <pre className="code-block" style={{ maxHeight: 'calc(100vh - 280px)' }}>
              {grammar.slr_table_raw}
            </pre>
          )}
        </>
      )}
    </div>
  )
}
