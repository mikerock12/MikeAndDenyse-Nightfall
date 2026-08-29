using UnityEngine;

namespace Nightfall
{
    /// <summary>
    /// Touch controls in 960×540 GUI space. Sampled from screen pixels, not uGUI
    /// (OnGUI draws the world on top of the canvas, so EventSystem pads never fire).
    /// </summary>
    public static class PlayTouch
    {
        public static Ctl Current = new();
        public static bool PauseDown;

        static bool _jumpHeld, _atkHeld, _pauseHeld;

        // round pads in 960×540 space; the view draws discs on exactly these rects
        public static readonly Rect Left = new(22, 372, 116, 116);
        public static readonly Rect Right = new(146, 372, 116, 116);
        public static readonly Rect Down = new(84, 268, 94, 94);
        public static readonly Rect Jump = new(796, 356, 132, 132);
        public static readonly Rect Attack = new(658, 396, 112, 112);
        public static readonly Rect Pause = new(16, 10, 78, 46);

        public static void Reset()
        {
            Current = new Ctl();
            PauseDown = false;
            _jumpHeld = _atkHeld = _pauseHeld = false;
        }

        public static void Sample()
        {
            bool l = false, r = false, d = false, jn = false, atk = false, pause = false;
            int n = Input.touchCount;
            if (n > 0)
            {
                for (int i = 0; i < n; i++)
                {
                    var t = Input.GetTouch(i);
                    if (t.phase == TouchPhase.Ended || t.phase == TouchPhase.Canceled) continue;
                    Hit(ScreenToGui(t.position), ref l, ref r, ref d, ref jn, ref atk, ref pause);
                }
            }
            else if (Input.GetMouseButton(0))
                Hit(ScreenToGui(Input.mousePosition), ref l, ref r, ref d, ref jn, ref atk, ref pause);

            Current = new Ctl
            {
                L = l,
                R = r,
                D = d,
                Jn = jn,
                Jp = jn && !_jumpHeld,
                Ap = atk && !_atkHeld
            };
            PauseDown = pause && !_pauseHeld;
            _jumpHeld = jn;
            _atkHeld = atk;
            _pauseHeld = pause;
        }

        public static Vector2 ScreenToGui(Vector2 screen)
        {
            float w = Mathf.Max(1, Screen.width);
            float h = Mathf.Max(1, Screen.height);
            return new Vector2(screen.x / w * T.ViewW, (h - screen.y) / h * T.ViewH);
        }

        /// <summary>Round pads with a few pixels of forgiveness, matching the discs the view draws.</summary>
        static bool Round(Rect r, Vector2 p, float pad = 8f)
        {
            float cx = r.x + r.width * 0.5f, cy = r.y + r.height * 0.5f;
            float rad = r.width * 0.5f + pad;
            float dx = p.x - cx, dy = p.y - cy;
            return dx * dx + dy * dy <= rad * rad;
        }

        static void Hit(Vector2 p, ref bool l, ref bool r, ref bool d, ref bool jn, ref bool atk, ref bool pause)
        {
            if (Round(Left, p)) l = true;
            if (Round(Right, p)) r = true;
            if (Round(Down, p)) d = true;
            if (Round(Jump, p)) jn = true;
            if (Round(Attack, p)) atk = true;
            if (Pause.Contains(p)) pause = true;
        }
    }
}
