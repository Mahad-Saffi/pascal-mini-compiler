import type { Scope } from '../types'

interface Props { scopes: Scope[] }

function KindBadge({ kind }: { kind: string }) {
  return (
    <span className="badge badge-gray">
      {kind}
    </span>
  )
}

export default function SymbolTableView({ scopes }: Props) {
  // Filter out builtin scope for cleaner display (it's always the same)
  const userScopes = scopes.filter(s => s.entries.some(e => e.kind !== 'builtin'))
  const builtinScope = scopes.find(s => s.entries.every(e => e.kind === 'builtin'))

  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 16 }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <div className="section-label" style={{ margin: 0 }}>Symbol Table</div>
        <span style={{ fontSize: 12, color: 'var(--text-4)' }}>
          Module 4.5 · hash-based, nested scopes · populated during RDP
        </span>
        <span className="badge badge-gray">{scopes.length} scope(s)</span>
      </div>

      {/* User scopes */}
      {userScopes.map(scope => (
        <div key={scope.level} style={{
          border: '1px solid var(--border)',
          borderRadius: 'var(--radius-lg)',
          overflow: 'hidden',
        }}>
          <div style={{
            padding: '8px 16px',
            background: 'var(--bg-subtle)',
            borderBottom: '1px solid var(--border)',
            display: 'flex', alignItems: 'center', gap: 10,
          }}>
            <span style={{ fontWeight: 700, fontSize: 13 }}>Scope {scope.level}</span>
            {scope.label && (
              <span style={{ fontSize: 12, color: 'var(--text-3)' }}>{scope.label}</span>
            )}
            <span className="badge badge-gray" style={{ marginLeft: 'auto' }}>
              {scope.entries.length} entries
            </span>
          </div>

          {scope.entries.length === 0 ? (
            <div style={{ padding: '12px 16px', color: 'var(--text-4)', fontSize: 13 }}>
              (empty scope)
            </div>
          ) : (
            <table className="data-table">
              <thead>
                <tr>
                  <th>Name</th>
                  <th>Kind</th>
                  <th>Type / Info</th>
                  <th>Declared at</th>
                </tr>
              </thead>
              <tbody>
                {scope.entries.map((e, i) => (
                  <tr key={i}>
                    <td className="mono" style={{ fontWeight: 600, fontSize: 13 }}>{e.name}</td>
                    <td><KindBadge kind={e.kind} /></td>
                    <td className="mono" style={{ fontSize: 12 }}>{e.type_info}</td>
                    <td className="mono text-muted" style={{ fontSize: 12 }}>
                      {e.line != null ? `line ${e.line}` : '—'}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      ))}

      {/* Built-in scope (collapsed) */}
      {builtinScope && builtinScope.entries.length > 0 && (
        <details style={{ border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', overflow: 'hidden' }}>
          <summary style={{
            padding: '8px 16px', cursor: 'pointer',
            background: 'var(--bg-subtle)',
            fontWeight: 600, fontSize: 13,
            borderBottom: '1px solid var(--border)',
            listStyle: 'none', display: 'flex', alignItems: 'center', gap: 8,
          }}>
            <span>▶ Built-in Scope (scope {builtinScope.level})</span>
            <span className="badge badge-gray">{builtinScope.entries.length} entries</span>
          </summary>
          <table className="data-table">
            <thead>
              <tr><th>Name</th><th>Kind</th><th>Type / Info</th></tr>
            </thead>
            <tbody>
              {builtinScope.entries.map((e, i) => (
                <tr key={i}>
                  <td className="mono" style={{ fontWeight: 600, fontSize: 13 }}>{e.name}</td>
                  <td><KindBadge kind={e.kind} /></td>
                  <td className="mono" style={{ fontSize: 12 }}>{e.type_info}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </details>
      )}

      {scopes.length === 0 && (
        <div className="empty-state">
          <p>No symbol table — parsing failed before analysis</p>
        </div>
      )}
    </div>
  )
}
