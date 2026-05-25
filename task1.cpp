using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace SteamboatLifeSimulation
{
    enum EventPriority
    {
        Low = 3,
        Medium = 2,
        High = 1,
        Critical = 0
    }

    class SteamboatEvent : IComparable<SteamboatEvent>
    {
        public string Name { get; set; }
        public EventPriority Priority { get; set; }
        public int DurationMs { get; set; }
        public DateTime Timestamp { get; set; }

        public SteamboatEvent(string name, EventPriority priority, int durationMs)
        {
            Name = name;
            Priority = priority;
            DurationMs = durationMs;
            Timestamp = DateTime.Now;
        }

        public int CompareTo(SteamboatEvent other)
        {
            if (other == null) return 1;
            int priorityCompare = this.Priority.CompareTo(other.Priority);
            if (priorityCompare != 0) return priorityCompare;
            return this.Timestamp.CompareTo(other.Timestamp);
        }
    }

    class PriorityQueue<T> where T : IComparable<T>
    {
        private List<T> data = new List<T>();

        public int Count => data.Count;

        public void Enqueue(T item)
        {
            data.Add(item);
            int ci = data.Count - 1;
            while (ci > 0)
            {
                int pi = (ci - 1) / 2;
                if (data[ci].CompareTo(data[pi]) >= 0) break;
                T tmp = data[ci]; data[ci] = data[pi]; data[pi] = tmp;
                ci = pi;
            }
        }

        public T Dequeue()
        {
            int li = data.Count - 1;
            T frontItem = data[0];
            data[0] = data[li];
            data.RemoveAt(li);

            --li;
            int pi = 0;
            while (true)
            {
                int ci = pi * 2 + 1;
                if (ci > li) break;
                int rc = ci + 1;
                if (rc <= li && data[rc].CompareTo(data[ci]) < 0)
                    ci = rc;
                if (data[pi].CompareTo(data[ci]) <= 0) break;
                T tmp = data[pi]; data[pi] = data[ci]; data[ci] = tmp;
                pi = ci;
            }
            return frontItem;
        }
    }

    class SimulationStats
    {
        public int TotalEventsProcessed { get; set; } = 0;
        public Dictionary<string, int> EventCounts { get; set; } = new Dictionary<string, int>();
        public Dictionary<string, double> TotalDurationPerEvent { get; set; } = new Dictionary<string, double>();

        public void TrackEvent(string name, double durationSeconds)
        {
            TotalEventsProcessed++;
            if (!EventCounts.ContainsKey(name))
            {
                EventCounts[name] = 0;
                TotalDurationPerEvent[name] = 0;
            }
            EventCounts[name]++;
            TotalDurationPerEvent[name] += durationSeconds;
        }
    }

    class Program
    {
        private static PriorityQueue<SteamboatEvent> eventQueue = new PriorityQueue<SteamboatEvent>();
        private static readonly object queueLock = new object();
        private static SimulationStats stats = new SimulationStats();
        private static bool isRunning = true;

        static async Task Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;
            
            int simulationDurationSeconds = 15;
            TimeSpan runDuration = TimeSpan.FromSeconds(simulationDurationSeconds);
            DateTime endTime = DateTime.Now.Add(runDuration);

            Task producerTask = Task.Run(() => GenerateEventsAsync(endTime));
            Task consumerTask = Task.Run(() => ProcessEventsAsync());

            await Task.Delay(runDuration);
            isRunning = false;

            await Task.WhenAll(producerTask, consumerTask);

            DisplayStatistics(simulationDurationSeconds);
        }

        static async Task GenerateEventsAsync(DateTime endTime)
        {
            Random rand = new Random();
            string[] eventTypes = { "Завантаження вугілля", "Посадка пасажирів", "Технічний огляд", "Штормове попередження", "Вихід у море" };

            while (DateTime.Now < endTime)
            {
                string name = eventTypes[rand.Next(eventTypes.Length)];
                EventPriority priority = EventPriority.Medium;

                if (name == "Штормове попередження") priority = EventPriority.Critical;
                else if (name == "Технічний огляд") priority = EventPriority.High;
                else if (name == "Посадка пасажирів") priority = EventPriority.Low;

                int duration = rand.Next(500, 1500);

                SteamboatEvent newEvent = new SteamboatEvent(name, priority, duration);

                lock (queueLock)
                {
                    eventQueue.Enqueue(newEvent);
                }

                await Task.Delay(rand.Next(300, 800));
            }
        }

        static async Task ProcessEventsAsync()
        {
            while (isRunning || eventQueue.Count > 0)
            {
                SteamboatEvent currentEvent = null;

                lock (queueLock)
                {
                    if (eventQueue.Count > 0)
                    {
                        currentEvent = eventQueue.Dequeue();
                    }
                }

                if (currentEvent != null)
                {
                    Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Початок: {currentEvent.Name} (Пріоритет: {currentEvent.Priority})");
                    
                    await Task.Delay(currentEvent.DurationMs);
                    
                    double durationInSeconds = currentEvent.DurationMs / 1000.0;
                    
                    lock (stats)
                    {
                        stats.TrackEvent(currentEvent.Name, durationInSeconds);
                    }

                    Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Завершено: {currentEvent.Name}");
                }
                else
                {
                    await Task.Delay(100);
                }
            }
        }

        static void DisplayStatistics(int seconds)
        {
            Console.WriteLine("\n" + new string('-', 50));
            Console.WriteLine($"СТАТИСТИКА МОДЕЛЮВАННЯ ЗА {seconds} СЕКУНД");
            Console.WriteLine(new string('-', 50));
            Console.WriteLine($"Всього оброблено подій: {stats.TotalEventsProcessed}");
            Console.WriteLine(new string('-', 50));
            Console.WriteLine($"{"Назва події",-25} | {"Кількість",-10} | {"Загальний час (сек)",-20}");
            Console.WriteLine(new string('-', 50));

            foreach (var kvp in stats.EventCounts)
            {
                string name = kvp.Key;
                int count = kvp.Value;
                double duration = stats.TotalDurationPerEvent[name];
                Console.WriteLine($"{name,-25} | {count,-10} | {duration,-20:F2}");
            }
            Console.WriteLine(new string('-', 50));
        }
    }
}
