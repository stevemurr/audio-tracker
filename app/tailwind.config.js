/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        // Deep studio console palette
        console: {
          950: '#05070a',
          900: '#0a0e14',
          850: '#0f1419',
          800: '#151c24',
          700: '#1e2832',
          600: '#2a3641',
        },
        // VU meter amber/orange
        vu: {
          400: '#fbbf24',
          500: '#f59e0b',
          600: '#d97706',
        },
        // Cool cyan accent
        signal: {
          400: '#22d3ee',
          500: '#06b6d4',
          600: '#0891b2',
        },
        // Waveform green
        wave: {
          400: '#4ade80',
          500: '#22c55e',
          600: '#16a34a',
        },
        // Frequency magenta
        freq: {
          400: '#e879f9',
          500: '#d946ef',
          600: '#c026d3',
        }
      },
      fontFamily: {
        display: ['Syncopate', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
        body: ['Outfit', 'sans-serif'],
      },
      backgroundImage: {
        'grid-pattern': `linear-gradient(rgba(34, 211, 238, 0.03) 1px, transparent 1px),
                         linear-gradient(90deg, rgba(34, 211, 238, 0.03) 1px, transparent 1px)`,
        'gradient-radial': 'radial-gradient(ellipse at center, var(--tw-gradient-stops))',
      },
      backgroundSize: {
        'grid': '40px 40px',
      },
      animation: {
        'pulse-slow': 'pulse 3s cubic-bezier(0.4, 0, 0.6, 1) infinite',
        'glow': 'glow 2s ease-in-out infinite alternate',
        'scan': 'scan 4s linear infinite',
      },
      keyframes: {
        glow: {
          '0%': { opacity: '0.5' },
          '100%': { opacity: '1' },
        },
        scan: {
          '0%': { transform: 'translateY(-100%)' },
          '100%': { transform: 'translateY(100%)' },
        }
      },
      boxShadow: {
        'glass': '0 8px 32px 0 rgba(0, 0, 0, 0.37)',
        'glass-inset': 'inset 0 1px 1px 0 rgba(255, 255, 255, 0.05)',
        'vu-glow': '0 0 20px rgba(251, 191, 36, 0.3)',
        'signal-glow': '0 0 20px rgba(34, 211, 238, 0.3)',
        'wave-glow': '0 0 20px rgba(74, 222, 128, 0.3)',
        'freq-glow': '0 0 20px rgba(232, 121, 249, 0.3)',
      }
    },
  },
  plugins: [],
}
