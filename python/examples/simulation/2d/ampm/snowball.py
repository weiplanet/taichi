from taichi.two_d import *
from taichi.misc.util import *

if __name__ == '__main__':
    scale = 4
    simulator = MPMSimulator(res=(80 * scale, 45 * scale), simulation_time=2, frame_dt=2e-2, base_delta_t=1e-6,
                             async=True, debug_input=(16, 10, 0, 0))

    simulator.add_event(-1, lambda s: s.add_particles_sphere(Vector(0.72, 0.45), 0.10, 'ep', compression=0.8,
                                                             velocity=Vector(1.0, 0.0), theta_s=0.002))

    levelset = simulator.create_levelset()
    levelset.add_polygon([(0.05, 0.05), (1.73, 0.05), (1.73, 0.95), (0.05, 0.95)], True)
    simulator.set_levelset(levelset)
    window = SimulationWindow(640, simulator, color_schemes['snow'], levelset_supersampling=2, show_images=True)
