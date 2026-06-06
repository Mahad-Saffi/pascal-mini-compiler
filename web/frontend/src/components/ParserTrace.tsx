import { useState } from 'react'
import type { CSSProperties } from 'react'
import type { LL1Row, LRRow } from '../types'

interface Props {
  kind: 'll1' | 'lr'
  rows: LL1Row[] | LRRow[]
  raw: string
}

function actionStyle(action: string): CSSProperties {
  if (action === 'accept')
    return { fontWeight: 700, background: '#111827', color: '#fff', padding: '0 4px', borderRadius: 2 }
  if (action.startsWith('shift'))
    return { fontWeight: 700 }
  if (action.startsWith('reduce'))
    return { fontStyle: 'italic', background: 'var(--bg-subtle)' }
  if (action.startsWith('output'))
    return { color: 'var(--text-3)', fontStyle: 'italic' }
  if (action.startsWith('error') || action.startsWith('->'))
    return { color: 'var(--text-3)' }
  return {}
}

export default function ParserTrace({ kind, rows, raw }: Props) {
  const [view, setView] = useState<'table' | 'raw'>('table')
  const [page, setPage] = useState(0)
  const PAGE = 50

  const totalPages = Math.ceil(rows.length / PAGE)
  const visible = rows.slice(page * PAGE, (page + 1) * PAGE)

  const title = kind === 'll1'
    ? 'Predictive LL(1) Parser · Module 4.3 · table-driven over Transformed Grammar'
    : 'LR SLR(1) Parser · Module 4.4 · shift-reduce over Original Grammar'

  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 12 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10, flexWrap: 'wrap' }}>
        <div className="section-label" style={{ margin: 0 }}>
          {kind === 'll1' ? 'LL(1) Predictive Parser' : 'LR SLR(1) Parser'}
        </div>
        <span style={{ fontSize: 12, color: 'var(--text-4)', flex: 1 }}>{title}</span>
        <span className="badge badge-gray">{rows.length} steps</span>
        <div style={{
          display: 'flex', border: '1px solid var(--border)',
          borderRadius: 'var(--radius-sm)', overflow: 'hidden',
        }}>
          {(['table', 'raw'] as const).map(v => (
            <button key={v} onClick={() => setView(v)} style={{
              padding: '3px 10px', fontSize: 12,
              background: view === v ? '#111827' : '#fff',
              color: view === v ? '#fff' : 'var(--text-2)',
              border: 'none', cursor: 'pointer',
              fontFamily: 'var(--font-sans)',
            }}>
              {v === 'table' ? 'Table' : 'Raw'}
            </button>
          ))}
        </div>
      </div>

      {view === 'raw' ? (
        <pre className="code-block" style={{ maxHeight: 'calc(100vh - 240px)' }}>{raw}</pre>
      ) : (
        <>
          <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
            <div style={{ overflowX: 'auto', maxHeight: 'calc(100vh - 300px)', overflowY: 'auto' }}>
              {kind === 'll1' ? (
                <table className="data-table">
                  <thead>
                    <tr>
                      <th>#</th>
                      <th>Stack (bottom → top)</th>
                      <th>Input</th>
                      <th>Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {(visible as LL1Row[]).map((row, i) => (
                      <tr key={i}>
                        <td className="mono text-muted" style={{ fontSize: 11, width: 40 }}>{page * PAGE + i + 1}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 260, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{row.stack}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 180, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{row.input}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 300, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', ...actionStyle(row.action) }}>{row.action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              ) : (
                <table className="data-table">
                  <thead>
                    <tr>
                      <th>#</th>
                      <th>State Stack</th>
                      <th>Symbol Stack</th>
                      <th>Input</th>
                      <th>Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {(visible as LRRow[]).map((row, i) => (
                      <tr key={i}>
                        <td className="mono text-muted" style={{ fontSize: 11, width: 40 }}>{page * PAGE + i + 1}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 160, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{row.states}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 180, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{row.symbols}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 140, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{row.input}</td>
                        <td className="mono" style={{ fontSize: 11, maxWidth: 280, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', ...actionStyle(row.action) }}>{row.action}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )}
            </div>
          </div>

          {totalPages > 1 && (
            <div style={{ display: 'flex', alignItems: 'center', gap: 8, justifyContent: 'center' }}>
              <button
                onClick={() => setPage(p => Math.max(0, p - 1))}
                disabled={page === 0}
                style={{ padding: '4px 12px', fontSize: 12, border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)', cursor: 'pointer', background: '#fff', fontFamily: 'var(--font-sans)' }}
              >
                Prev
              </button>
              <span style={{ fontSize: 12, color: 'var(--text-3)' }}>
                {page + 1} / {totalPages}  ({rows.length} rows)
              </span>
              <button
                onClick={() => setPage(p => Math.min(totalPages - 1, p + 1))}
                disabled={page >= totalPages - 1}
                style={{ padding: '4px 12px', fontSize: 12, border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)', cursor: 'pointer', background: '#fff', fontFamily: 'var(--font-sans)' }}
              >
                Next
              </button>
            </div>
          )}

          <div style={{ display: 'flex', gap: 12, flexWrap: 'wrap', fontSize: 11, color: 'var(--text-3)', alignItems: 'center' }}>
            <span><strong>bold</strong> = shift</span>
            <span><em>italic/gray bg</em> = reduce</span>
            <span style={{ background: '#111827', color: '#fff', padding: '0 5px', borderRadius: 2 }}>accept</span>
            <span>= accepted</span>
          </div>
        </>
      )}
    </div>
  )
}
