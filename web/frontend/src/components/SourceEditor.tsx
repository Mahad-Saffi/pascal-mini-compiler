import { useState, useRef } from 'react'
import type { Sample } from '../types'

interface Props {
  source: string
  samples: Sample[]
  loading: boolean
  onSourceChange: (s: string) => void
  onCompile: () => void
}

export default function SourceEditor({ source, samples, loading, onSourceChange, onCompile }: Props) {
  const [selected, setSelected] = useState('')
  const textareaRef = useRef<HTMLTextAreaElement>(null)

  const handleSampleChange = (name: string) => {
    setSelected(name)
    const s = samples.find(s => s.name === name)
    if (s) onSourceChange(s.content)
  }

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      e.preventDefault()
      onCompile()
    }
    // Tab → insert 3 spaces
    if (e.key === 'Tab') {
      e.preventDefault()
      const el = textareaRef.current!
      const start = el.selectionStart
      const end   = el.selectionEnd
      const newVal = source.slice(0, start) + '   ' + source.slice(end)
      onSourceChange(newVal)
      requestAnimationFrame(() => {
        el.selectionStart = el.selectionEnd = start + 3
      })
    }
  }

  const lineCount = source.split('\n').length

  return (
    <div style={{
      width: 380, flexShrink: 0,
      display: 'flex', flexDirection: 'column',
      background: '#fff', overflow: 'hidden',
    }}>
      {/* Toolbar */}
      <div style={{
        padding: '8px 12px',
        borderBottom: '1px solid var(--border)',
        display: 'flex', alignItems: 'center', gap: 8,
        flexShrink: 0,
      }}>
        <span className="section-label" style={{ margin: 0 }}>Source</span>
        <div style={{ flex: 1 }} />
        {samples.length > 0 && (
          <select
            value={selected}
            onChange={e => handleSampleChange(e.target.value)}
            style={{
              padding: '3px 8px', fontSize: 12,
              border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)',
              background: 'var(--bg-subtle)', color: 'var(--text-2)',
              fontFamily: 'var(--font-sans)', cursor: 'pointer',
            }}
          >
            <option value="">Load sample…</option>
            {samples.map(s => (
              <option key={s.name} value={s.name}>{s.name}</option>
            ))}
          </select>
        )}
      </div>

      {/* Editor */}
      <div style={{ flex: 1, position: 'relative', overflow: 'hidden', display: 'flex' }}>
        {/* Line numbers */}
        <div style={{
          width: 40, flexShrink: 0,
          padding: '12px 0',
          background: 'var(--bg-subtle)',
          borderRight: '1px solid var(--border)',
          overflow: 'hidden',
          fontFamily: 'var(--font-mono)',
          fontSize: 12,
          lineHeight: '21px',
          color: 'var(--text-4)',
          textAlign: 'right',
          userSelect: 'none',
        }}>
          {Array.from({ length: lineCount }, (_, i) => (
            <div key={i} style={{ paddingRight: 8 }}>{i + 1}</div>
          ))}
        </div>

        <textarea
          ref={textareaRef}
          value={source}
          onChange={e => { onSourceChange(e.target.value); setSelected('') }}
          onKeyDown={handleKeyDown}
          spellCheck={false}
          disabled={loading}
          style={{
            flex: 1,
            padding: '12px 14px',
            fontFamily: 'var(--font-mono)',
            fontSize: 13,
            lineHeight: '21px',
            border: 'none',
            outline: 'none',
            resize: 'none',
            background: '#fff',
            color: 'var(--text-1)',
            tabSize: 3,
            overflowY: 'auto',
          }}
        />
      </div>

      {/* Footer hint */}
      <div style={{
        padding: '5px 12px',
        borderTop: '1px solid var(--border)',
        fontSize: 11, color: 'var(--text-4)',
        display: 'flex', gap: 12, flexShrink: 0,
      }}>
        <span>{lineCount} lines</span>
        <span>Ctrl+Enter to compile</span>
      </div>
    </div>
  )
}
