#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <map>

using namespace std;
/**
 * @struct Proceso
 * @brief Representa un proceso dentro del sistema operativo para la planificación.
 *
 * Contiene todos los datos de entrada (tiempos y colas) y las métricas de rendimiento
 * calculadas durante y al final de la simulación.
 *
 * @author Jeferson Stiven Trullot Rivas
 * @version 1.0
 */
struct Proceso {
    string etiqueta;
    int burst_time_original;
    int burst_time_restante;
    int arrival_time;
    int queue;
    int priority;
    int completion_time = 0;
    int turnaround_time = 0;
    int waiting_time = 0;
    int response_time = -1; 

 /**
     * @brief Constructor para inicializar un nuevo proceso.
     *
     * Inicializa los tiempos que necesita cada proceso en la CPU y restante con el mismo valor (bt).
     *
     * @param e Etiqueta del proceso.
     * @param bt Tiempo de ejecución (Burst Time).
     * @param at Tiempo de llegada (Arrival Time).
     * @param q ID de la cola de planificación.
     * @param p Prioridad del proceso.
     */

    Proceso(const string& e, int bt, int at, int q, int p) 
        : etiqueta(e), burst_time_original(bt), burst_time_restante(bt), arrival_time(at), queue(q), priority(p) {}
};

/**
 * @brief Lee los datos de los procesos desde un archivo de texto.
 *
 * El archivo debe contener los campos separados por punto y coma (etiqueta;bt;at;queue;priority).
 * Implementa manejo de errores de apertura y conversión de tipos, incluyendo la limpieza
 * de caracteres BOM y espacios en blanco.
 *
 * @param filename Nombre del archivo de entrada.
 * @return vector<Proceso> Un vector de procesos cargados y listos para la ejecución.
 * @version 1.0
 */
vector<Proceso> leerProcesos(const string& filename) {
    vector<Proceso> procesos;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir el archivo de entrada: " << filename << endl;
        return procesos;
    }

    string line;
    int line_count = 0;
    
    while (getline(file, line)) {
        line_count++;
         // Manejo y eliminación del BOM (Byte Order Mark) en archivos UTF-8
        if (line_count == 1 && line.size() >= 3 && (unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
            line = line.substr(3);
        }
        
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        if (line.empty() || line[0] == '#') {
            continue;
        }

        stringstream ss(line);
        string segment;
        vector<string> seglist;
        
        while (getline(ss, segment, ';')) {
            segment.erase(0, segment.find_first_not_of(" \t\n\r\f\v"));
            segment.erase(segment.find_last_not_of(" \t\n\r\f\v") + 1);
            seglist.push_back(segment);
        }

        if (seglist.size() >= 5) {
            try {
                string etiqueta = seglist[0];
                int bt = stoi(seglist[1]);
                int at = stoi(seglist[2]);
                int q = stoi(seglist[3]);
                int p = stoi(seglist[4]);
                
                procesos.emplace_back(etiqueta, bt, at, q, p);
            } catch (const std::exception& e) {
                cerr << "Error de conversion en la linea " << line_count << ": " << line << endl;
            }
        }
    }
    file.close();
    return procesos;
}

/**
 * @brief Ejecuta el Planificador Multi-Level Queue (MLQ).
 *
 * Implementa un esquema de prioridad fija (Q1 > Q2 > Q3) donde cada cola usa
 * Round Robin (RR) con un quantum predefinido (Q1=1, Q2=3, Q3=2).
 *
 * @param procesos Vector de procesos cargados desde el archivo de entrada.
 * @return vector<Proceso> El vector de procesos con todas las métricas calculadas.
 * @version 1.0
 */
vector<Proceso> ejecutarPlanificador(vector<Proceso> procesos) {
    if (procesos.empty()) {
        cout << "No hay procesos para planificar." << endl;
        return procesos;
    }
    cout << "\n--- INICIO DEL DESPACHADOR MLQ (RR q=1, RR q=3, RR q=2) ---" << endl;
    
    const map<int, int> QUANTUM_MAP = {
        {1, 1},
        {2, 3},
        {3, 2}
    };

    vector<Proceso> procesos_ejecución = procesos;
    
    sort(procesos_ejecución.begin(), procesos_ejecución.end(), [](const Proceso& a, const Proceso& b) {
        return a.arrival_time < b.arrival_time;
    });

    map<int, queue<int>> ready_queues; 
    ready_queues[1] = queue<int>();
    ready_queues[2] = queue<int>();
    ready_queues[3] = queue<int>();

    vector<Proceso> resultados;
    int tiempo_actual = 0;
    int procesos_ingresados_idx = 0;
    int procesos_terminados = 0;
    int total_procesos = procesos_ejecución.size();

    int proceso_ejecutando_idx = -1; 

    while (procesos_terminados < total_procesos) {
        
        while (procesos_ingresados_idx < total_procesos && procesos_ejecución[procesos_ingresados_idx].arrival_time <= tiempo_actual) {
            int idx = procesos_ingresados_idx;
            ready_queues[procesos_ejecución[idx].queue].push(idx);
            procesos_ingresados_idx++;
        }

        int proximo_proceso_idx = -1;
        int proxima_cola = -1;
        
        if (!ready_queues[1].empty()) {
            proximo_proceso_idx = ready_queues[1].front();
            proxima_cola = 1;
        } else if (!ready_queues[2].empty()) {
            proximo_proceso_idx = ready_queues[2].front();
            proxima_cola = 2;
        } else if (!ready_queues[3].empty()) {
            proximo_proceso_idx = ready_queues[3].front();
            proxima_cola = 3;
        } else if (proceso_ejecutando_idx == -1) {
            if (procesos_ingresados_idx < total_procesos) {
                int tiempo_anterior = tiempo_actual;
                tiempo_actual = procesos_ejecución[procesos_ingresados_idx].arrival_time;
                if (tiempo_actual > tiempo_anterior) {
                    cout << "[T=" << tiempo_anterior << " a " << tiempo_actual << "] IDLE (CPU Inactiva). Esperando el arribo del Proceso " << procesos_ejecución[procesos_ingresados_idx].etiqueta << "." << endl;
                }
                continue;
            } else {
                break; 
            }
        }
        
        if (proximo_proceso_idx != -1) {
            
            proceso_ejecutando_idx = proximo_proceso_idx;
            ready_queues[proxima_cola].pop();

            Proceso& p = procesos_ejecución[proceso_ejecutando_idx];
            int quantum = QUANTUM_MAP.at(proxima_cola);
            
            int tiempo_ejecutar = min(p.burst_time_restante, quantum);
            int tiempo_finalizacion_ejecucion = tiempo_actual + tiempo_ejecutar;

            if (p.response_time == -1) {
                p.response_time = tiempo_actual - p.arrival_time;
            }

            p.burst_time_restante -= tiempo_ejecutar;
            
            cout << "[T=" << tiempo_actual << "] DESPACHO: Proceso " << p.etiqueta 
                 << " (Q" << proxima_cola << ", q=" << quantum << ") por " 
                 << tiempo_ejecutar << "u. (Restante: " << p.burst_time_restante << ")" << endl;

            tiempo_actual = tiempo_finalizacion_ejecucion;

            while (procesos_ingresados_idx < total_procesos && procesos_ejecución[procesos_ingresados_idx].arrival_time <= tiempo_actual) {
                int idx = procesos_ingresados_idx;
                int cola_proceso_nuevo = procesos_ejecución[idx].queue;
                ready_queues[cola_proceso_nuevo].push(idx);
                procesos_ingresados_idx++;
            }
            
            if (p.burst_time_restante == 0) {
                p.completion_time = tiempo_actual;
                p.turnaround_time = p.completion_time - p.arrival_time;
                p.waiting_time = p.turnaround_time - p.burst_time_original;
                procesos_terminados++;
                cout << "[T=" << tiempo_actual << "] COMPLETADO: Proceso " << p.etiqueta << endl;
                proceso_ejecutando_idx = -1;
            } else {
                ready_queues[proxima_cola].push(proceso_ejecutando_idx);
                proceso_ejecutando_idx = -1;
            }
        } else {
            break;
        }

    }
    
    cout << "--- FIN DEL PLANIFICADOR MLQ (Tiempo Total de Uso: " << tiempo_actual << ") ---" << endl;
    return procesos_ejecución;
}

/**
 * @brief Escribe los resultados de la ejecución y las métricas de rendimiento promedio.
 *
 * Genera un archivo de texto con el detalle de cada proceso y un resumen de promedios
 * (TAT, WT, RT).
 *
 * @param filename Nombre del archivo de salida donde se guardarán los resultados.
 * @param resultados Vector de procesos con todas las métricas calculadas.
 * @return bool Retorna true si la escritura fue exitosa, false en caso de error.
 * @version 1.0
 */

bool escribirResultados(const string& filename, const vector<Proceso>& resultados) {
    ofstream outfile(filename);

    if (!outfile.is_open()) {
        cerr << "ERROR FATAL: No se pudo abrir el archivo de salida: " << filename << endl;
        return false;
    }

    outfile << "Etiqueta\tBT\tAT\tCT\tTAT\tWT\tRT\tQ\tPrioridad" << endl;
    
    for (const auto& p : resultados) {
        outfile << p.etiqueta << "\t"
                << p.burst_time_original << "\t"
                << p.arrival_time << "\t"
                << p.completion_time << "\t"
                << p.turnaround_time << "\t"
                << p.waiting_time << "\t"
                << p.response_time << "\t"
                << p.queue << "\t"
                << p.priority << endl;
    }

    double total_tat = 0, total_wt = 0, total_rt = 0;
    for (const auto& p : resultados) {
        total_tat += p.turnaround_time;
        total_wt += p.waiting_time;
        total_rt += p.response_time;
    }
    double count = resultados.size();
    
    outfile << "\n--- MÉTRICAS DE RENDIMIENTO ---" << endl;
    outfile << "TAT Promedio: " << fixed << setprecision(2) << total_tat / count << endl;
    outfile << "WT Promedio: " << fixed << setprecision(2) << total_wt / count << endl;
    outfile << "RT Promedio: " << fixed << setprecision(2) << total_rt / count << endl;

    outfile.close();

    cout << "\nRegistros de rendimiento generados en " << filename << endl;
    return true;
}

/**
 * @brief Función principal del programa.
 *
 * Realiza la carga de datos, la ejecución del planificador y la escritura de resultados.
 *
 * @param argc Conteo de argumentos pasados por línea de comandos.
 * @param argv Vector de strings con los argumentos (argv[1] debe ser el archivo de entrada).
 * @return int Código de salida (0 si es exitoso, 1 en caso de error).
 * @version 1.0
 */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <archivo_de_entrada.txt>" << endl;
        return 1;
    }

    string input_filename = argv[1];
    string output_filename = "mlq001_output_log.txt";

    vector<Proceso> procesos = leerProcesos(input_filename);

    if (procesos.empty()) {
        cerr << "No se pudieron cargar procesos. Terminando el planificador." << endl;
        return 1;
    }

    vector<Proceso> resultados = ejecutarPlanificador(procesos);

    if (!escribirResultados(output_filename, resultados)) {
        return 1;
    }

    return 0;
}