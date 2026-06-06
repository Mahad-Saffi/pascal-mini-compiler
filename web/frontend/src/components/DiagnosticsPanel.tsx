import type { Diagnostic, ErrorSummary } from '../types'

interface Props {
  diagnostics: Diagnostic[]
  summary: ErrorSummary
}

export default function DiagnosticsPanel({ diagnostics, summary }: Props) {
  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 16 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <div className="section-label" style={{ margin: 0 }}>Semantic Analysis</div>
        <span style={{ fontSize: 12, color: 'var(--text-4)' }}>
          Module 4.6 · type checking · declaration checking · error handler (bonus)
        </span>
      </div>

      {/* Summary cards */}
      <div style={{ display: 'flex', gap: 12 }}>
        <div style={{
          flex: 1, padding: '12px 16px',
          border: '1px solid var(--border)',
          borderRadius: 'var(--radius)',
          display: 'flex', alignItems: 'center', gap: 10,
        }}>
          <span style={{ fontSize: 24, fontWeight: 700, color: 'var(--text-1)' }}>
            {summary.errors}
          </span>
          <div>
            <div style={{ fontWeight: 600, fontSize: 13, color: 'var(--text-1)' }}>
              {summary.errors === 0 ? 'No errors' : `Error${summary.errors !== 1 ? 's' : ''}`}
            </div>
            <div style={{ fontSize: 11, color: 'var(--text-4)' }}>lexical · syntactic · semantic</div>
          </div>
        </div>

        <div style={{
          flex: 1, padding: '12px 16px',
          border: '1px solid var(--border)',
          borderRadius: 'var(--radius)',
          display: 'flex', alignItems: 'center', gap: 10,
        }}>
          <span style={{ fontSize: 24, fontWeight: 700, color: 'var(--text-3)' }}>
            {summary.warnings}
          </span>
          <div>
            <div style={{ fontWeight: 600, fontSize: 13, color: 'var(--text-3)' }}>
              {summary.warnings === 0 ? 'No warnings' : `Warning${summary.warnings !== 1 ? 's' : ''}`}
            </div>
          </div>
        </div>
      </div>

      {/* Diagnostic list */}
      {diagnostics.length === 0 ? (
        <div style={{
          padding: '16px 20px',
          border: '1px solid var(--border)',
          borderLeft: '3px solid var(--text-1)',
          borderRadius: 'var(--radius)',
          background: '#fff',
          color: 'var(--text-1)',
          fontSize: 14, fontWeight: 600,
          display: 'flex', alignItems: 'center', gap: 10,
        }}>
          <span style={{ fontSize: 20 }}>✓</span>
          <span>Program is semantically valid. No errors detected.</span>
        </div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
          {diagnostics.map((d, i) => {
            const isErr = d.severity !== 'warning'
            return (
              <div key={i} style={{
                padding: '10px 14px',
                border: '1px solid var(--border)',
                borderLeft: `3px solid ${isErr ? 'var(--text-1)' : 'var(--text-3)'}`,
                borderRadius: 'var(--radius)',
                background: '#fff',
                display: 'flex', gap: 12, alignItems: 'flex-start',
              }}>
                <span style={{ fontSize: 16, lineHeight: 1.2 }}>{isErr ? '✗' : '⚠'}</span>
                <div style={{ flex: 1 }}>
                  <div style={{ display: 'flex', gap: 8, marginBottom: 4, flexWrap: 'wrap' }}>
                    <span className="badge badge-gray">{d.phase}</span>
                    <span className="mono" style={{ fontSize: 12, color: 'var(--text-3)' }}>
                      line {d.line}, col {d.col}
                    </span>
                  </div>
                  <div style={{ fontSize: 13, color: 'var(--text-1)', lineHeight: 1.4 }}>
                    {d.message}
                  </div>
                </div>
              </div>
            )
          })}
        </div>
      )}
    </div>
  )
}
