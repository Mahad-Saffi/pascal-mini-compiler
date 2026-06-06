import type { CompileResult, GrammarData, Sample } from './types'

const BASE = '/api'

export async function compileSource(source: string): Promise<CompileResult> {
  const res = await fetch(`${BASE}/compile`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source }),
  })
  if (!res.ok) throw new Error(`Server error ${res.status}`)
  return res.json()
}

export async function fetchGrammar(): Promise<GrammarData> {
  const res = await fetch(`${BASE}/grammar`)
  if (!res.ok) throw new Error(`Server error ${res.status}`)
  return res.json()
}

export async function fetchSamples(): Promise<Sample[]> {
  const res = await fetch(`${BASE}/samples`)
  if (!res.ok) throw new Error(`Server error ${res.status}`)
  const data = await res.json()
  return data.samples
}
