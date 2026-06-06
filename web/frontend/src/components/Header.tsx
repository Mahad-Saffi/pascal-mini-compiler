import type { CompileResult } from '../types'

interface Props {
  result: CompileResult | null
  loading: boolean
  onCompile: () => void
}

export default function Header({ result, loading, onCompile }: Props) {
  const errors   = result?.error_summary?.errors   ?? 0
  const warnings = result?.error_summary?.warnings ?? 0
  const agree    = result?.agreement?.agree

  return (
    <header style={{
      display: 'flex', alignItems: 'center', gap: 16,
      padding: '0 20px', height: 52,
      borderBottom: '1px solid var(--border)',
      background: '#fff', flexShrink: 0,
    }}>
      {/* Logo + title */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="var(--accent)" strokeWidth="2">
          <polyline points="16 18 22 12 16 6" /><polyline points="8 6 2 12 8 18" />
        </svg>
        <span style={{ fontWeight: 700, fontSize: 15, letterSpacing: '-0.02em', color: 'var(--text-1)' }}>
          Mini Pascal Compiler
        </span>
        <span className="badge badge-gray" style={{ fontSize: 10 }}>CS-471L</span>
      </div>

      <div style={{ flex: 1 }} />

      {/* Status badges */}
      {result && !result.error && (
        <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
          {errors === 0 && warnings === 0 && (
            <span className="badge badge-green">No errors</span>
          )}
          {errors > 0 && (
            <span className="badge badge-red">{errors} error{errors !== 1 ? 's' : ''}</span>
          )}
          {warnings > 0 && (
            <span className="badge badge-amber">{warnings} warning{warnings !== 1 ? 's' : ''}</span>
          )}
          {agree !== undefined && (
            <span className={`badge ${agree ? 'badge-green' : 'badge-red'}`}>
              Parsers {agree ? 'agree' : 'disagree'}
            </span>
          )}
        </div>
      )}

      {/* Compile button */}
      <button
        onClick={onCompile}
        disabled={loading}
        style={{
          display: 'flex', alignItems: 'center', gap: 6,
          padding: '6px 14px',
          background: loading ? 'var(--bg-subtle)' : 'var(--accent)',
          color: loading ? 'var(--text-3)' : '#fff',
          border: 'none', borderRadius: 'var(--radius)',
          fontFamily: 'var(--font-sans)', fontWeight: 600, fontSize: 13,
          cursor: loading ? 'not-allowed' : 'pointer',
          transition: 'background 0.15s',
        }}
      >
        {loading ? (
          <>
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5"
              style={{ animation: 'spin 0.8s linear infinite' }}>
              <path d="M21 12a9 9 0 1 1-6.219-8.56" />
            </svg>
            Compiling…
          </>
        ) : (
          <>
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
              <polygon points="5 3 19 12 5 21 5 3" />
            </svg>
            Compile
          </>
        )}
      </button>

      <style>{`
        @keyframes spin { to { transform: rotate(360deg); } }
      `}</style>
    </header>
  )
}
