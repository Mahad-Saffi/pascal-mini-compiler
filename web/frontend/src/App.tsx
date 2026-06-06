import { useState, useCallback, useEffect } from 'react'
import type { CompileResult, GrammarData, Sample } from './types'
import { compileSource, fetchGrammar, fetchSamples } from './api'
import Header from './components/Header'
import SourceEditor from './components/SourceEditor'
import ResultPanel from './components/ResultPanel'

const DEFAULT_SOURCE = `{ Euclid's GCD — classic Subset-of-Pascal example }

program gcd (input, output);

var
   x, y : integer;

begin
   read(x);
   read(y);
   while x <> y do
      if x > y then
         x := x - y
      else
         y := y - x;
   write(x)
end.
`

export default function App() {
  const [source,   setSource]   = useState(DEFAULT_SOURCE)
  const [result,   setResult]   = useState<CompileResult | null>(null)
  const [grammar,  setGrammar]  = useState<GrammarData | null>(null)
  const [samples,  setSamples]  = useState<Sample[]>([])
  const [loading,  setLoading]  = useState(false)
  const [compErr,  setCompErr]  = useState<string | null>(null)

  useEffect(() => {
    fetchSamples().then(setSamples).catch(() => {})
  }, [])

  const handleCompile = useCallback(async () => {
    setLoading(true)
    setCompErr(null)
    try {
      const [res, gram] = await Promise.all([
        compileSource(source),
        grammar ? Promise.resolve(grammar) : fetchGrammar(),
      ])
      setResult(res)
      if (!grammar) setGrammar(gram)
    } catch (e) {
      setCompErr(e instanceof Error ? e.message : 'Compilation failed')
    } finally {
      setLoading(false)
    }
  }, [source, grammar])

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', overflow: 'hidden' }}>
      <Header
        result={result}
        loading={loading}
        onCompile={handleCompile}
      />
      <div style={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
        <SourceEditor
          source={source}
          samples={samples}
          loading={loading}
          onSourceChange={setSource}
          onCompile={handleCompile}
        />
        <div style={{ flex: 1, overflow: 'hidden', borderLeft: '1px solid var(--border)' }}>
          {compErr && (
            <div style={{
              margin: 16, padding: '10px 14px',
              background: 'var(--red-bg)', border: '1px solid #fecaca',
              borderRadius: 'var(--radius)', color: 'var(--red)', fontSize: 13,
            }}>
              {compErr}
            </div>
          )}
          {!result && !compErr && (
            <div className="empty-state" style={{ marginTop: 80 }}>
              <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
                <polyline points="16 18 22 12 16 6" /><polyline points="8 6 2 12 8 18" />
              </svg>
              <p style={{ fontWeight: 500, fontSize: 15, color: 'var(--text-2)' }}>
                Mini Pascal Compiler
              </p>
              <p>Edit the source code and click <strong>Compile</strong> to see all module outputs.</p>
              <p style={{ fontSize: 12 }}>Lexer · RDP · LL(1) · LR · Symbol Table · Semantics · Grammar</p>
            </div>
          )}
          {result && (
            <ResultPanel result={result} grammar={grammar} />
          )}
        </div>
      </div>
    </div>
  )
}
