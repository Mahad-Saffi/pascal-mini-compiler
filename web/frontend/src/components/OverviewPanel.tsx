import type { CompileResult } from '../types'

interface Props { result: CompileResult }

function VerdictCell({ accept }: { accept: boolean }) {
  return (
    <span className={accept ? 'verdict-accept' : 'verdict-reject'}>
      {accept ? 'accept' : 'reject'}
    </span>
  )
}

function Stat({ label, value, sub }: { label: string; value: string | number; sub?: string }) {
  return (
    <div style={{
      padding: '16px 20px',
      borderRight: '1px solid var(--border)',
      display: 'flex', flexDirection: 'column', gap: 2,
    }}>
      <div style={{ fontSize: 11, color: 'var(--text-3)', textTransform: 'uppercase', letterSpacing: '0.05em', fontWeight: 600 }}>
        {label}
      </div>
      <div style={{ fontSize: 22, fontWeight: 700, letterSpacing: '-0.02em', color: 'var(--text-1)' }}>
        {value}
      </div>
      {sub && <div style={{ fontSize: 11, color: 'var(--text-4)' }}>{sub}</div>}
    </div>
  )
}

export default function OverviewPanel({ result }: Props) {
  const { agreement, error_summary, tokens, symtab, diagnostics } = result
  const totalEntries = symtab.reduce((n, s) => n + s.entries.length, 0)

  return (
    <div style={{ padding: '20px 24px', display: 'flex', flexDirection: 'column', gap: 24 }}>
      {/* Stats strip */}
      <div style={{
        display: 'flex', borderRadius: 'var(--radius-lg)',
        border: '1px solid var(--border)', overflow: 'hidden',
        background: '#fff',
      }}>
        <Stat label="Tokens"       value={tokens.length - 1} sub="before EOF" />
        <Stat label="Symbol entries" value={totalEntries} sub={`${symtab.length} scope(s)`} />
        <Stat label="Errors"       value={error_summary.errors}   />
        <Stat label="Warnings"     value={error_summary.warnings} />
      </div>

      {/* Cross-parser agreement */}
      <div>
        <div className="section-label">Cross-parser agreement</div>
        <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
          <table className="data-table">
            <thead>
              <tr>
                <th>Parser</th>
                <th>Verdict</th>
                <th>Description</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td style={{ fontWeight: 600 }}>Recursive Descent</td>
                <td><VerdictCell accept={agreement.rdp} /></td>
                <td className="text-muted text-sm">Hand-written, one routine per non-terminal; builds the AST</td>
              </tr>
              <tr>
                <td style={{ fontWeight: 600 }}>Predictive LL(1)</td>
                <td><VerdictCell accept={agreement.ll1} /></td>
                <td className="text-muted text-sm">Table-driven with explicit stack; uses Transformed Grammar + LL(1) table</td>
              </tr>
              <tr>
                <td style={{ fontWeight: 600 }}>LR SLR(1)</td>
                <td><VerdictCell accept={agreement.lr} /></td>
                <td className="text-muted text-sm">Shift-reduce with ACTION/GOTO tables; uses Original Grammar</td>
              </tr>
            </tbody>
          </table>
          <div style={{
            padding: '10px 16px',
            background: 'var(--bg-subtle)',
            borderTop: '1px solid var(--border)',
            fontSize: 13, fontWeight: agreement.agree ? 700 : 400,
            color: 'var(--text-1)',
            display: 'flex', alignItems: 'center', gap: 8,
          }}>
            <span>{agreement.agree ? '✓' : '✗'}</span>
            <span>{agreement.verdict}</span>
          </div>
        </div>
      </div>

      {/* Error summary */}
      {diagnostics.length > 0 && (
        <div>
          <div className="section-label">Diagnostic summary</div>
          <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
            <table className="data-table">
              <thead>
                <tr><th>Phase</th><th>Sev.</th><th>Line</th><th>Col</th><th>Message</th></tr>
              </thead>
              <tbody>
                {diagnostics.map((d, i) => (
                  <tr key={i}>
                    <td><span className={`badge badge-${d.phase === 'semantic' ? 'amber' : 'blue'}`}>{d.phase}</span></td>
                    <td><span className={`badge badge-${d.severity === 'error' ? 'red' : 'amber'}`}>{d.severity}</span></td>
                    <td className="mono">{d.line}</td>
                    <td className="mono">{d.col}</td>
                    <td style={{ color: 'var(--text-2)' }}>{d.message}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {diagnostics.length === 0 && (
        <div style={{
          padding: '12px 16px',
          background: 'var(--bg-subtle)',
          border: '1px solid var(--border)',
          borderRadius: 'var(--radius)',
          color: 'var(--green)',
          fontSize: 13, fontWeight: 500,
          display: 'flex', alignItems: 'center', gap: 8,
        }}>
          <span>✓</span>
          <span>No errors or warnings. All three parsers agree.</span>
        </div>
      )}
    </div>
  )
}
