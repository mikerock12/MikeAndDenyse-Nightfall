using System.Collections.Generic;
using UnityEngine;

namespace Nightfall
{
    /// <summary>
    /// Tiny procedural sound bank. No audio files ship with the game, so every blip is
    /// synthesised once at boot and played through a pool of sources.
    /// </summary>
    public static class Sfx
    {
        const int Rate = 22050;
        static readonly Dictionary<string, AudioClip> Clips = new();
        static AudioSource[] _pool;
        static int _next;
        static bool _ready;
        static readonly Dictionary<string, float> LastPlay = new();

        public static void Ensure()
        {
            if (_ready) return;
            _ready = true;
            try
            {
                var go = new GameObject("Sfx");
                Object.DontDestroyOnLoad(go);
                _pool = new AudioSource[6];
                for (int i = 0; i < _pool.Length; i++)
                {
                    var s = go.AddComponent<AudioSource>();
                    s.playOnAwake = false;
                    s.spatialBlend = 0;
                    _pool[i] = s;
                }

                Clips["jump"] = Tone(0.13f, 300, 720, Wave.Square, 0.16f, 0);
                Clips["attack"] = Noise(0.10f, 0.20f, 0.55f);
                Clips["magic"] = Tone(0.20f, 520, 1180, Wave.Sine, 0.18f, 14);
                Clips["hit"] = Mix(Tone(0.09f, 220, 130, Wave.Square, 0.16f, 0), Noise(0.07f, 0.14f, 0.3f));
                Clips["stomp"] = Mix(Tone(0.12f, 150, 70, Wave.Square, 0.18f, 0), Noise(0.09f, 0.16f, 0.2f));
                Clips["hurt"] = Tone(0.26f, 420, 110, Wave.Saw, 0.20f, 6);
                Clips["die"] = Tone(0.70f, 330, 55, Wave.Saw, 0.22f, 4);
                Clips["coin"] = Arp(new[] { 880f, 1320f }, 0.07f, 0.15f, Wave.Square);
                Clips["heart"] = Arp(new[] { 660f, 880f, 1100f }, 0.07f, 0.15f, Wave.Sine);
                Clips["boss"] = Mix(Tone(0.85f, 90, 62, Wave.Saw, 0.22f, 3), Noise(0.5f, 0.10f, 0.12f));
                Clips["clear"] = Arp(new[] { 523f, 659f, 784f, 1046f }, 0.12f, 0.16f, Wave.Square);
                Clips["ui"] = Tone(0.06f, 620, 900, Wave.Square, 0.10f, 0);
            }
            catch { _pool = null; }
        }

        public static void Play(string id)
        {
            if (_pool == null || string.IsNullOrEmpty(id)) return;
            if (!Clips.TryGetValue(id, out var clip) || clip == null) return;
            // one-frame spam guard: a swarm of hits used to crackle
            float now = Time.unscaledTime;
            if (LastPlay.TryGetValue(id, out var t) && now - t < 0.035f) return;
            LastPlay[id] = now;
            var src = _pool[_next];
            _next = (_next + 1) % _pool.Length;
            src.pitch = 0.94f + Random.value * 0.12f;
            src.PlayOneShot(clip, 0.85f);
        }

        enum Wave { Sine, Square, Saw }

        static float Osc(Wave w, float phase)
        {
            phase -= Mathf.Floor(phase);
            return w switch
            {
                Wave.Square => phase < 0.5f ? 1f : -1f,
                Wave.Saw => phase * 2f - 1f,
                _ => Mathf.Sin(phase * Mathf.PI * 2f)
            };
        }

        static AudioClip Tone(float dur, float f0, float f1, Wave w, float vol, float vibrato)
        {
            int n = Mathf.Max(1, Mathf.RoundToInt(dur * Rate));
            var data = new float[n];
            float phase = 0;
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)n;
                float f = Mathf.Lerp(f0, f1, t);
                if (vibrato > 0) f += Mathf.Sin(t * dur * vibrato * 6.283f) * f * 0.05f;
                phase += f / Rate;
                float env = Mathf.Min(1f, t * 18f) * Mathf.Pow(1f - t, 1.6f);
                data[i] = Osc(w, phase) * env * vol;
            }
            return Clip(data);
        }

        static AudioClip Noise(float dur, float vol, float decay)
        {
            int n = Mathf.Max(1, Mathf.RoundToInt(dur * Rate));
            var data = new float[n];
            float lp = 0;
            for (int i = 0; i < n; i++)
            {
                float t = i / (float)n;
                float raw = Random.value * 2f - 1f;
                lp = Mathf.Lerp(lp, raw, 0.45f);
                float env = Mathf.Min(1f, t * 30f) * Mathf.Pow(1f - t, 1f + decay * 4f);
                data[i] = lp * env * vol;
            }
            return Clip(data);
        }

        static AudioClip Arp(float[] notes, float each, float vol, Wave w)
        {
            int per = Mathf.Max(1, Mathf.RoundToInt(each * Rate));
            var data = new float[per * notes.Length];
            for (int k = 0; k < notes.Length; k++)
            {
                float phase = 0;
                for (int i = 0; i < per; i++)
                {
                    float t = i / (float)per;
                    phase += notes[k] / Rate;
                    float env = Mathf.Min(1f, t * 20f) * Mathf.Pow(1f - t, 1.3f);
                    data[k * per + i] = Osc(w, phase) * env * vol;
                }
            }
            return Clip(data);
        }

        static AudioClip Mix(AudioClip a, AudioClip b)
        {
            var da = new float[a.samples]; a.GetData(da, 0);
            var db = new float[b.samples]; b.GetData(db, 0);
            int n = Mathf.Max(da.Length, db.Length);
            var outp = new float[n];
            for (int i = 0; i < n; i++)
            {
                float v = 0;
                if (i < da.Length) v += da[i];
                if (i < db.Length) v += db[i];
                outp[i] = Mathf.Clamp(v, -1f, 1f);
            }
            return Clip(outp);
        }

        static AudioClip Clip(float[] data)
        {
            var c = AudioClip.Create("sfx", data.Length, 1, Rate, false);
            c.SetData(data, 0);
            return c;
        }
    }
}
