import { useRef, useState, useEffect, useCallback } from 'react'
import Tree from 'react-d3-tree'
import type { RawNodeDatum, CustomNodeElementProps } from 'react-d3-tree'
import type { AstNode } from '../types'

// ── data helpers ─────────────────────────────────────────────────

function flattenSections(node: AstNode): AstNode {
  const flat: AstNode[] = []
  for (const child of node.children) {
    if (child.isSection) flat.push(...flattenSections(child).children)
    else flat.push(flattenSections(child))
  }
  return { ...node, children: flat }
}

function toD3(node: AstNode): RawNodeDatum {
  const n = flattenSections(node)
  return {
    name: n.type,
    attributes: n.label ? { value: n.label } : {},
    children: n.children.length > 0 ? n.children.map(toD3) : undefined,
  }
}

// ── node colours (automation-tool palette) ────────────────────────
// Solid background, white text — no CSS override can touch inline style.

// Solid backgrounds, white text with black stroke outline.
// paint-order: stroke fill draws stroke behind fill — clean outlined text on any bg.
const PALETTE: Record<string, string> = {
  Program:   '#2563eb',
  VarDecl:   '#16a34a',
  Compound:  '#7c3aed',
  If:        '#ea580c',
  While:     '#ea580c',
  For:       '#ea580c',
  Assign:    '#0891b2',
  Call:      '#0d9488',
  BinOp:     '#dc2626',
  UnaryOp:   '#dc2626',
  Var:       '#475569',
  Num:       '#475569',
  Function:  '#c026d3',
  Procedure: '#c026d3',
}

const LEGEND = [
  { color: '#2563eb', label: 'Program' },
  { color: '#16a34a', label: 'Declaration' },
  { color: '#7c3aed', label: 'Block / Compound' },
  { color: '#ea580c', label: 'Control flow' },
  { color: '#0d9488', label: 'Call / expression' },
  { color: '#475569', label: 'Leaf  (Var / Num)' },
]

// ── SVG node ──────────────────────────────────────────────────────

const NW   = 168
const NH   = 54
const SANS = "'Segoe UI', system-ui, -apple-system, Roboto, Helvetica, Arial, sans-serif"

function trunc(s: string, max = 22) {
  return s.length > max ? s.slice(0, max - 1) + '…' : s
}

function NodeShape({ nodeDatum, toggleNode }: CustomNodeElementProps) {
  const val       = (nodeDatum.attributes as Record<string, string> | undefined)?.value
  const collapsed = !!(nodeDatum as any).__rd3t?.collapsed
  const hasKids   = !!(nodeDatum.children?.length) || collapsed
  const bg        = PALETTE[nodeDatum.name] ?? '#475569'

  // Outlined text: paint-order draws the stroke behind the fill.
  // Black stroke + white fill = crisp white text on any background colour.
  const labelStyle: React.CSSProperties = {
    fill: '#ffffff',
    stroke: 'rgba(0,0,0,0.65)',
    strokeWidth: '2.5px',
    paintOrder: 'stroke fill',
    fontSize: '13px',
    fontWeight: '600',
    fontFamily: SANS,
  }
  const valueStyle: React.CSSProperties = {
    fill: 'rgba(255,255,255,0.92)',
    stroke: 'rgba(0,0,0,0.50)',
    strokeWidth: '2px',
    paintOrder: 'stroke fill',
    fontSize: '10.5px',
    fontWeight: '400',
    fontFamily: SANS,
  }

  return (
    <g onClick={hasKids ? toggleNode : undefined} style={{ cursor: hasKids ? 'pointer' : 'default' }}>
      <rect
        x={-NW / 2} y={-NH / 2}
        width={NW} height={NH} rx={10}
        fill={bg}
        stroke="rgba(255,255,255,0.20)"
        strokeWidth={1.5}
        strokeDasharray={collapsed ? '6 3' : undefined}
      />
      <text x={0} y={val ? -7 : 5} textAnchor="middle" style={labelStyle}>
        {nodeDatum.name}
      </text>
      {val && (
        <text x={0} y={12} textAnchor="middle" style={valueStyle}>
          {trunc(val)}
        </text>
      )}
      {collapsed && (
        <circle cx={NW / 2 - 10} cy={-NH / 2 + 10} r={3.5} fill="rgba(255,255,255,0.55)" />
      )}
    </g>
  )
}

// ── small overlay button ──────────────────────────────────────────

function OBtn({
  children, title, onClick,
}: { children: React.ReactNode; title: string; onClick: () => void }) {
  return (
    <button onClick={onClick} title={title} style={{
      width: 34, height: 34,
      background: 'rgba(15,23,42,0.07)',
      border: '1px solid rgba(15,23,42,0.14)',
      borderRadius: 7,
      cursor: 'pointer', userSelect: 'none' as const,
      display: 'flex', alignItems: 'center', justifyContent: 'center',
      color: '#1e293b', fontFamily: SANS,
    }}>
      {children}
    </button>
  )
}

// ── fullscreen icon ───────────────────────────────────────────────

function FsIcon({ exit }: { exit: boolean }) {
  return (
    <svg width="14" height="14" viewBox="0 0 24 24" fill="none"
      stroke="currentColor" strokeWidth="2" strokeLinecap="round">
      {exit ? (
        <>
          <polyline points="8 3 3 3 3 8"/>
          <polyline points="21 8 21 3 16 3"/>
          <polyline points="3 16 3 21 8 21"/>
          <polyline points="16 21 21 21 21 16"/>
        </>
      ) : (
        <>
          <polyline points="15 3 21 3 21 9"/>
          <polyline points="9 21 3 21 3 15"/>
          <line x1="21" y1="3" x2="14" y2="10"/>
          <line x1="3" y1="21" x2="10" y2="14"/>
        </>
      )}
    </svg>
  )
}

// ── main component ────────────────────────────────────────────────

const DEFAULT_ZOOM = 0.80
const STEP         = 1.35

interface Props { tree: AstNode | null; raw: string }

export default function AstTree({ tree, raw }: Props) {
  const [view,       setView]  = useState<'visual' | 'text'>('visual')
  const [resetKey,   setReset] = useState(0)
  const [zoomLevel,  setZoom]  = useState(DEFAULT_ZOOM)
  const [fullscreen, setFs]    = useState(false)
  const containerRef            = useRef<HTMLDivElement>(null)
  const [translate,  setTrans] = useState({ x: 500, y: 60 })

  useEffect(() => {
    const recenter = () => {
      if (!containerRef.current) return
      const w = containerRef.current.getBoundingClientRect().width
      setTrans({ x: Math.round(w / 2) || 500, y: 60 })
    }
    recenter()
    document.addEventListener('fullscreenchange', recenter)
    return () => document.removeEventListener('fullscreenchange', recenter)
  }, [tree, resetKey])

  useEffect(() => {
    const h = () => setFs(!!document.fullscreenElement)
    document.addEventListener('fullscreenchange', h)
    return () => document.removeEventListener('fullscreenchange', h)
  }, [])

  const toggleFs = () => {
    if (!containerRef.current) return
    if (!document.fullscreenElement) {
      containerRef.current.requestFullscreen().catch(() => {})
    } else {
      document.exitFullscreen()
    }
  }

  const d3data     = tree ? toD3(tree) : null
  const renderNode = useCallback(
    (props: CustomNodeElementProps) => <NodeShape {...props} />, []
  )

  const zoomIn  = () => { setZoom(z => Math.min(3,    Math.round(z * STEP  * 100) / 100)); setReset(k => k + 1) }
  const zoomOut = () => { setZoom(z => Math.max(0.15, Math.round(z / STEP  * 100) / 100)); setReset(k => k + 1) }
  const reset   = () => { setZoom(DEFAULT_ZOOM); setReset(k => k + 1) }

  return (
    <div style={{ padding: '16px 20px', display: 'flex', flexDirection: 'column', gap: 12 }}>

      {/* ── toolbar ── */}
      <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
        <div className="section-label" style={{ margin: 0 }}>Abstract Syntax Tree</div>
        <span style={{ fontSize: 12, color: 'var(--text-4)' }}>
          Recursive Descent Parser · Module 4.2
        </span>
        <div style={{ flex: 1 }} />

        <button
          onClick={toggleFs}
          title={fullscreen ? 'Exit fullscreen' : 'Fullscreen'}
          style={{
            display: 'flex', alignItems: 'center', gap: 5,
            padding: '3px 10px', fontSize: 12,
            border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)',
            background: '#fff', color: 'var(--text-2)',
            cursor: 'pointer', fontFamily: 'var(--font-sans)',
          }}
        >
          <FsIcon exit={fullscreen} />
          {fullscreen ? 'Exit' : 'Fullscreen'}
        </button>

        <div style={{
          display: 'flex', border: '1px solid var(--border)',
          borderRadius: 'var(--radius-sm)', overflow: 'hidden',
        }}>
          {(['visual', 'text'] as const).map(v => (
            <button key={v} onClick={() => setView(v)} style={{
              padding: '3px 10px', fontSize: 12,
              background: view === v ? '#111827' : '#fff',
              color:      view === v ? '#fff'    : 'var(--text-2)',
              border: 'none', cursor: 'pointer', fontFamily: 'var(--font-sans)',
            }}>{v === 'visual' ? 'Visual' : 'Text'}</button>
          ))}
        </div>
      </div>

      {/* ── visual tree ── */}
      {view === 'visual' ? (
        <div
          ref={containerRef}
          style={{
            border: '1px solid var(--border)',
            borderRadius: 'var(--radius-lg)',
            height: 'calc(100vh - 260px)',
            overflow: 'hidden',
            // light dot-grid canvas — exactly like n8n / automation tools
            background: '#f4f6f9',
            backgroundImage: 'radial-gradient(circle, #c8d0db 1px, transparent 1px)',
            backgroundSize: '24px 24px',
            position: 'relative',
          }}
        >
          {d3data ? (
            <>
              <Tree
                key={resetKey}
                data={d3data}
                orientation="vertical"
                translate={translate}
                nodeSize={{ x: 210, y: 130 }}
                separation={{ siblings: 1.2, nonSiblings: 1.6 }}
                renderCustomNodeElement={renderNode}
                initialDepth={2}
                pathFunc="step"
                zoom={zoomLevel}
                collapsible
              />

              {/* zoom / fullscreen overlay */}
              <div style={{
                position: 'absolute', bottom: 16, right: 16, zIndex: 10,
                display: 'flex', flexDirection: 'column', gap: 5,
              }}>
                <OBtn title="Zoom in"    onClick={zoomIn}>
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                    <line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/>
                  </svg>
                </OBtn>
                <OBtn title="Zoom out"   onClick={zoomOut}>
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5">
                    <line x1="5" y1="12" x2="19" y2="12"/>
                  </svg>
                </OBtn>
                <OBtn title="Reset view" onClick={reset}>
                  <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/>
                  </svg>
                </OBtn>
                <OBtn title={fullscreen ? 'Exit fullscreen' : 'Fullscreen'} onClick={toggleFs}>
                  <FsIcon exit={fullscreen} />
                </OBtn>
              </div>

              {/* zoom % */}
              <div style={{
                position: 'absolute', bottom: 18, left: 16,
                fontSize: 11, color: 'rgba(15,23,42,0.30)',
                fontFamily: SANS, pointerEvents: 'none',
              }}>
                {Math.round(zoomLevel * 100)}%
              </div>

              {/* legend */}
              <div style={{
                position: 'absolute', top: 12, left: 12, zIndex: 5,
                background: 'rgba(255,255,255,0.82)',
                backdropFilter: 'blur(6px)',
                border: '1px solid rgba(0,0,0,0.08)',
                borderRadius: 8,
                padding: '8px 10px',
                display: 'flex', flexDirection: 'column', gap: 5,
              }}>
                {LEGEND.map(({ color, label }) => (
                  <div key={label} style={{ display: 'flex', alignItems: 'center', gap: 7 }}>
                    <div style={{
                      width: 10, height: 10, borderRadius: 3,
                      background: color, flexShrink: 0,
                    }} />
                    <span style={{ fontSize: 10.5, color: '#374151', fontFamily: SANS }}>
                      {label}
                    </span>
                  </div>
                ))}
              </div>
            </>
          ) : (
            <div style={{
              position: 'absolute', inset: 0,
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              color: '#94a3b8', fontSize: 13, fontFamily: SANS,
            }}>
              No AST — compile first or check for parse errors
            </div>
          )}
        </div>
      ) : (
        <pre className="code-block" style={{ maxHeight: 'calc(100vh - 280px)' }}>
          {raw || '(no output)'}
        </pre>
      )}

      <p style={{ fontSize: 11, color: 'var(--text-4)' }}>
        Drag to pan · scroll to zoom · +/− to scale · click nodes to collapse/expand
      </p>
    </div>
  )
}
