export interface Token {
  line: number
  col: number
  type: string
  lexeme: string
  terminal: string
}

export interface AstNode {
  id: number
  type: string
  label: string
  isSection: boolean
  children: AstNode[]
}

export interface SymEntry {
  name: string
  kind: string
  type_info: string
  line: number | null
}

export interface Scope {
  level: number
  label: string
  entries: SymEntry[]
}

export interface Diagnostic {
  phase: string
  severity: string
  line: number
  col: number
  message: string
}

export interface ErrorSummary {
  errors: number
  warnings: number
}

export interface LL1Row {
  stack: string
  input: string
  action: string
}

export interface LRRow {
  states: string
  symbols: string
  input: string
  action: string
}

export interface Agreement {
  rdp: boolean
  ll1: boolean
  lr: boolean
  verdict: string
  agree: boolean
}

export interface FirstFollowEntry {
  nonterminal: string
  first: string[]
  follow: string[]
}

export interface CompileResult {
  success: boolean
  error?: string
  tokens: Token[]
  ast_text: string
  ast_tree: AstNode | null
  ll1_trace: LL1Row[]
  ll1_raw: string
  lr_trace: LRRow[]
  lr_raw: string
  symtab: Scope[]
  diagnostics: Diagnostic[]
  error_summary: ErrorSummary
  agreement: Agreement
  rdp_raw: string
}

export interface GrammarData {
  grammar_raw: string
  first_follow: FirstFollowEntry[]
  first_follow_raw: string
  ll1_table_raw: string
  slr_table_raw: string
}

export interface Sample {
  name: string
  content: string
}
