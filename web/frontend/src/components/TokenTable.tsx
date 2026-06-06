import type { Token } from '../types'

interface Props { tokens: Token[] }

function TypeBadge({ type }: { type: string }) {
  return (
    <span className="badge badge-gray" style={{ fontFamily: 'var(--font-mono)' }}>
      {type}
    </span>
  )
}

export default function TokenTable({ tokens }: Props) {
  const visible = tokens.filter(t => t.type !== 'EndOfFile')

  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 12 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <div className="section-label" style={{ margin: 0 }}>Token stream</div>
        <span className="badge badge-gray">{visible.length} tokens</span>
        <span style={{ fontSize: 12, color: 'var(--text-4)', marginLeft: 4 }}>
          Lexer Module 4.1 · double-buffered character reader
        </span>
      </div>

      <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
        <div style={{ overflowX: 'auto', maxHeight: 'calc(100vh - 220px)', overflowY: 'auto' }}>
          <table className="data-table">
            <thead>
              <tr>
                <th>#</th>
                <th>Line</th>
                <th>Col</th>
                <th>Type</th>
                <th>Lexeme</th>
                <th>Grammar Terminal</th>
              </tr>
            </thead>
            <tbody>
              {visible.map((tok, i) => (
                <tr key={i}>
                  <td className="mono text-muted" style={{ fontSize: 11 }}>{i + 1}</td>
                  <td className="mono" style={{ textAlign: 'right', color: 'var(--text-3)' }}>{tok.line}</td>
                  <td className="mono" style={{ textAlign: 'right', color: 'var(--text-3)' }}>{tok.col}</td>
                  <td><TypeBadge type={tok.type} /></td>
                  <td className="mono" style={{ fontSize: 12 }}>{tok.lexeme}</td>
                  <td className="mono text-muted" style={{ fontSize: 12 }}>{tok.terminal}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  )
}
