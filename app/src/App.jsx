import { useState, useEffect, useCallback, useMemo } from 'react'
import { motion } from 'framer-motion'
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  ReferenceLine
} from 'recharts'

const API_BASE = 'http://localhost:9091/api/audio'

// Utility to calculate mean
const mean = (arr) => {
  if (!arr || arr.length === 0) return 0
  return arr.reduce((sum, val) => sum + val, 0) / arr.length
}

// Utility to calculate min/max with padding
const getRange = (arr, padding = 0.1) => {
  if (!arr || arr.length === 0) return [0, 100]
  const validValues = arr.filter(v => v !== null && v !== undefined && !isNaN(v))
  if (validValues.length === 0) return [0, 100]
  const min = Math.min(...validValues)
  const max = Math.max(...validValues)
  const range = max - min || 1
  return [min - range * padding, max + range * padding]
}

// Individual metric chart component
const MetricChart = ({
  data,
  dataKey,
  label,
  unit,
  color,
  gradientId,
  domain,
  delay = 0,
  isLast = false
}) => {
  const chartData = useMemo(() => data, [data])

  // Calculate dynamic domain if not provided
  const yDomain = useMemo(() => {
    if (domain) return domain
    const values = chartData.map(d => d[dataKey]).filter(v => v != null)
    return getRange(values, 0.15)
  }, [chartData, dataKey, domain])

  const avg = useMemo(() => {
    const values = chartData.map(d => d[dataKey]).filter(v => v != null)
    return mean(values)
  }, [chartData, dataKey])

  const CustomTooltip = ({ active, payload }) => {
    if (!active || !payload || !payload.length) return null
    const value = payload[0]?.value
    return (
      <div className="glass-panel px-3 py-2 rounded-lg border border-white/10">
        <p className="font-mono text-sm" style={{ color }}>
          {typeof value === 'number' ? value.toFixed(2) : value} {unit}
        </p>
      </div>
    )
  }

  return (
    <motion.div
      initial={{ opacity: 0, x: -20 }}
      animate={{ opacity: 1, x: 0 }}
      transition={{ duration: 0.6, delay, ease: [0.16, 1, 0.3, 1] }}
      className="relative"
    >
      {/* Chart header */}
      <div className="flex items-center justify-between mb-2 px-1">
        <div className="flex items-center gap-3">
          <div className="w-2 h-2 rounded-full" style={{ backgroundColor: color }} />
          <span className="text-xs font-mono uppercase tracking-[0.15em] text-white/50">
            {label}
          </span>
        </div>
        <div className="flex items-center gap-4">
          <span className="text-xs font-mono text-white/30">
            avg: <span style={{ color }}>{avg.toFixed(1)}</span> {unit}
          </span>
        </div>
      </div>

      {/* Chart */}
      <div className="h-[140px] relative">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={chartData} margin={{ top: 5, right: 10, left: 0, bottom: 5 }}>
            <defs>
              <linearGradient id={gradientId} x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={color} stopOpacity={0.3} />
                <stop offset="100%" stopColor={color} stopOpacity={0} />
              </linearGradient>
              <filter id={`glow-${gradientId}`} x="-50%" y="-50%" width="200%" height="200%">
                <feGaussianBlur stdDeviation="2" result="coloredBlur"/>
                <feMerge>
                  <feMergeNode in="coloredBlur"/>
                  <feMergeNode in="SourceGraphic"/>
                </feMerge>
              </filter>
            </defs>

            <CartesianGrid
              strokeDasharray="3 3"
              stroke="rgba(255,255,255,0.03)"
              vertical={false}
            />

            {/* Average reference line */}
            <ReferenceLine
              y={avg}
              stroke={color}
              strokeDasharray="4 4"
              strokeOpacity={0.3}
            />

            <XAxis
              dataKey="index"
              axisLine={false}
              tickLine={false}
              tick={isLast ? { fill: 'rgba(255,255,255,0.25)', fontSize: 10, fontFamily: 'JetBrains Mono' } : false}
              tickFormatter={(val) => val % 20 === 0 ? val : ''}
              height={isLast ? 20 : 0}
            />

            <YAxis
              domain={yDomain}
              axisLine={false}
              tickLine={false}
              tick={{ fill: 'rgba(255,255,255,0.25)', fontSize: 10, fontFamily: 'JetBrains Mono' }}
              tickFormatter={(val) => val.toFixed(0)}
              width={45}
            />

            <Tooltip content={<CustomTooltip />} />

            <Area
              type="monotone"
              dataKey={dataKey}
              stroke={color}
              strokeWidth={2}
              fill={`url(#${gradientId})`}
              filter={`url(#glow-${gradientId})`}
              dot={false}
              activeDot={{ r: 4, fill: color, stroke: color, strokeWidth: 2 }}
              isAnimationActive={false}
            />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </motion.div>
  )
}

// Metric card component with current value display
const MetricCard = ({ label, value, unit, color, glowClass, icon, min, max, delay = 0 }) => {
  const numValue = parseFloat(value) || 0
  const displayValue = isNaN(numValue) ? '—' : numValue.toFixed(1)

  // Calculate percentage for the bar (handle negative values like dB)
  const minVal = min ?? 0
  const maxVal = max ?? 1000
  const percentage = Math.min(100, Math.max(0, ((numValue - minVal) / (maxVal - minVal)) * 100))

  return (
    <motion.div
      initial={{ opacity: 0, x: 50 }}
      animate={{ opacity: 1, x: 0 }}
      transition={{ duration: 0.6, delay, ease: [0.16, 1, 0.3, 1] }}
      className="glass-panel rounded-2xl p-5 relative overflow-hidden group"
    >
      {/* Glow effect on hover */}
      <div className={`absolute inset-0 opacity-0 group-hover:opacity-100 transition-opacity duration-500 ${glowClass} blur-3xl`} />

      {/* Content */}
      <div className="relative z-10">
        <div className="flex items-center justify-between mb-3">
          <span className="text-[10px] font-mono uppercase tracking-[0.2em] text-white/40">
            {label}
          </span>
          <span className={`text-base ${color}`}>{icon}</span>
        </div>

        <div className="flex items-baseline gap-2">
          <motion.span
            key={displayValue}
            initial={{ opacity: 0, y: -10 }}
            animate={{ opacity: 1, y: 0 }}
            className={`text-3xl font-display font-bold tracking-tight ${color}`}
          >
            {displayValue}
          </motion.span>
          <span className="text-xs font-mono text-white/30">{unit}</span>
        </div>

        {/* Mini bar indicator */}
        <div className="mt-3 h-1 bg-white/5 rounded-full overflow-hidden">
          <motion.div
            className={`h-full rounded-full ${color.replace('text-', 'bg-')}`}
            initial={{ width: 0 }}
            animate={{ width: `${percentage}%` }}
            transition={{ duration: 0.5, ease: 'easeOut' }}
          />
        </div>

        {/* Range labels */}
        <div className="flex justify-between mt-1">
          <span className="text-[9px] font-mono text-white/20">{minVal}</span>
          <span className="text-[9px] font-mono text-white/20">{maxVal}</span>
        </div>
      </div>
    </motion.div>
  )
}

// Status indicator
const StatusIndicator = ({ connected }) => (
  <div className="flex items-center gap-2">
    <motion.div
      className={`w-2 h-2 rounded-full ${connected ? 'bg-wave-400' : 'bg-red-500'}`}
      animate={{ opacity: connected ? [1, 0.5, 1] : 1 }}
      transition={{ duration: 2, repeat: Infinity }}
    />
    <span className="text-xs font-mono text-white/40">
      {connected ? 'RECEIVING' : 'WAITING'}
    </span>
  </div>
)

// Main App component
export default function App() {
  const [chartData, setChartData] = useState([])
  const [averages, setAverages] = useState({ f0: 0, rms: 0, centroid: 0 })
  const [ranges, setRanges] = useState({ f0: [0, 600], rms: [-60, 0], centroid: [0, 5000] })
  const [connected, setConnected] = useState(false)
  const [lastUpdate, setLastUpdate] = useState(null)

  const fetchData = useCallback(async () => {
    try {
      const response = await fetch(`${API_BASE}/chart`)
      const json = await response.json()

      if (json.datasets && json.datasets.length >= 3) {
        const f0Data = json.datasets[0].data || []
        const rmsData = json.datasets[1].data || []
        const centroidData = json.datasets[2].data || []

        // Transform data for Recharts
        const maxLength = Math.max(f0Data.length, rmsData.length, centroidData.length)
        const transformed = Array.from({ length: maxLength }, (_, i) => ({
          index: i,
          f0: f0Data[i] ?? null,
          rms: rmsData[i] ?? null,
          centroid: centroidData[i] ?? null,
        }))

        setChartData(transformed)
        setAverages({
          f0: mean(f0Data),
          rms: mean(rmsData),
          centroid: mean(centroidData),
        })

        // Update ranges dynamically
        setRanges({
          f0: getRange(f0Data, 0.15),
          rms: getRange(rmsData, 0.15),
          centroid: getRange(centroidData, 0.15),
        })

        setConnected(true)
        setLastUpdate(new Date())
      }
    } catch (err) {
      console.error('Fetch error:', err)
      setConnected(false)
    }
  }, [])

  useEffect(() => {
    fetchData()
    const interval = setInterval(fetchData, 1500)
    return () => clearInterval(interval)
  }, [fetchData])

  return (
    <div className="min-h-screen bg-console-950 text-white overflow-hidden">
      {/* Background effects */}
      <div className="fixed inset-0 bg-grid-pattern bg-grid opacity-100" />
      <div className="fixed inset-0 bg-gradient-radial from-console-800/20 via-transparent to-transparent" />

      {/* Ambient glow orbs */}
      <div className="fixed top-1/4 -left-32 w-96 h-96 bg-signal-500/10 rounded-full blur-[128px] pointer-events-none" />
      <div className="fixed bottom-1/4 -right-32 w-96 h-96 bg-freq-500/10 rounded-full blur-[128px] pointer-events-none" />

      {/* Main content */}
      <div className="relative z-10 p-6 lg:p-8">
        {/* Header */}
        <motion.header
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, ease: [0.16, 1, 0.3, 1] }}
          className="flex items-center justify-between mb-8"
        >
          <div className="flex items-center gap-6">
            <div className="flex items-center gap-3">
              <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-signal-400 to-freq-500 flex items-center justify-center">
                <svg className="w-5 h-5 text-console-950" fill="none" viewBox="0 0 24 24" stroke="currentColor" strokeWidth={2.5}>
                  <path strokeLinecap="round" strokeLinejoin="round" d="M9 19V6l12-3v13M9 19c0 1.105-1.343 2-3 2s-3-.895-3-2 1.343-2 3-2 3 .895 3 2zm12-3c0 1.105-1.343 2-3 2s-3-.895-3-2 1.343-2 3-2 3 .895 3 2zM9 10l12-3" />
                </svg>
              </div>
              <div>
                <h1 className="font-display text-xl font-bold tracking-wide">AUDIO TRACKER</h1>
                <p className="text-xs font-mono text-white/40 tracking-wider">REAL-TIME ANALYSIS</p>
              </div>
            </div>
          </div>

          <div className="flex items-center gap-6">
            <StatusIndicator connected={connected} />
            {lastUpdate && (
              <span className="text-xs font-mono text-white/30">
                {lastUpdate.toLocaleTimeString()}
              </span>
            )}
          </div>
        </motion.header>

        {/* Main grid layout */}
        <div className="grid grid-cols-1 xl:grid-cols-4 gap-6">
          {/* Stacked charts section - spans 3 columns */}
          <motion.div
            initial={{ opacity: 0, y: 30 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ duration: 0.8, delay: 0.1, ease: [0.16, 1, 0.3, 1] }}
            className="xl:col-span-3 glass-panel rounded-3xl p-6"
          >
            <div className="flex items-center justify-between mb-6">
              <div>
                <h2 className="font-display text-sm font-bold tracking-[0.15em] text-white/60 mb-1">
                  WAVEFORM ANALYSIS
                </h2>
                <p className="text-xs font-mono text-white/30">
                  {chartData.length} samples • Individual scales
                </p>
              </div>
            </div>

            {/* Stacked mini charts */}
            <div className="space-y-4">
              <MetricChart
                data={chartData}
                dataKey="f0"
                label="Fundamental Frequency (F0)"
                unit="Hz"
                color="#fbbf24"
                gradientId="gradientF0"
                domain={ranges.f0}
                delay={0.15}
              />

              <MetricChart
                data={chartData}
                dataKey="rms"
                label="RMS Energy"
                unit="dB"
                color="#4ade80"
                gradientId="gradientRMS"
                domain={ranges.rms}
                delay={0.25}
              />

              <MetricChart
                data={chartData}
                dataKey="centroid"
                label="Spectral Centroid"
                unit="Hz"
                color="#e879f9"
                gradientId="gradientCentroid"
                domain={ranges.centroid}
                delay={0.35}
                isLast={true}
              />
            </div>
          </motion.div>

          {/* Metrics sidebar */}
          <div className="flex flex-col gap-4">
            <MetricCard
              label="Fundamental"
              value={averages.f0}
              unit="Hz"
              color="text-vu-400"
              glowClass="bg-vu-400/20"
              icon="♪"
              min={Math.floor(ranges.f0[0])}
              max={Math.ceil(ranges.f0[1])}
              delay={0.2}
            />

            <MetricCard
              label="RMS Energy"
              value={averages.rms}
              unit="dB"
              color="text-wave-400"
              glowClass="bg-wave-400/20"
              icon="◈"
              min={Math.floor(ranges.rms[0])}
              max={Math.ceil(ranges.rms[1])}
              delay={0.3}
            />

            <MetricCard
              label="Centroid"
              value={averages.centroid}
              unit="Hz"
              color="text-freq-400"
              glowClass="bg-freq-400/20"
              icon="◇"
              min={Math.floor(ranges.centroid[0])}
              max={Math.ceil(ranges.centroid[1])}
              delay={0.4}
            />

            {/* Info panel */}
            <motion.div
              initial={{ opacity: 0, x: 50 }}
              animate={{ opacity: 1, x: 0 }}
              transition={{ duration: 0.6, delay: 0.5, ease: [0.16, 1, 0.3, 1] }}
              className="glass-panel rounded-2xl p-5 flex-1"
            >
              <h3 className="text-[10px] font-mono uppercase tracking-[0.2em] text-white/40 mb-3">
                About Metrics
              </h3>
              <div className="space-y-3 text-[11px] font-mono text-white/50 leading-relaxed">
                <p>
                  <span className="text-vu-400">F0</span> — Pitch detected via autocorrelation (60-600 Hz typical)
                </p>
                <p>
                  <span className="text-wave-400">RMS</span> — Energy level in decibels (0 dB = max)
                </p>
                <p>
                  <span className="text-freq-400">Centroid</span> — Brightness from FFT (higher = brighter)
                </p>
              </div>
            </motion.div>
          </div>
        </div>
      </div>
    </div>
  )
}
