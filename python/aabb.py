#/!/usr/bin/env python2

import pygame


def cross(v1, v2):
    return v2[0]*v1[1] - v2[1]*v1[0]

# e.g. intersect([rayP, rayV], [lineP1, lineP2])
def intersect(ray, line, screen):
    pygame.draw.line(screen, (255, 0, 0),
                     (ray[0][0], ray[0][1]),
                     (ray[0][0] + 50*ray[1][0],
                      ray[0][1] + 50*ray[1][1] ) )
    pygame.draw.line(screen, (0, 255, 0),
                     (line[0][0], line[0][1]),
                     (line[1][0], line[1][1] ) )
    
    v1 = [ray[0][0] - line[0][0], ray[0][1] - line[0][1]]
    v2 = [line[1][0] - line[0][0], line[1][1] - line[0][1]]
    v3 = [-ray[1][1], ray[1][0]]

    dot = v2[0]*v3[0] + v2[1]*v3[1]
    if abs(dot) < 0.000001:
        return 100

    t1 = cross(v2, v1) / dot
    t2 = (v1[0]*v3[0] + v1[1]*v3[1])/dot

    if t1 > 0.0 and (t2 >= 0.0 and t2 <= 1.0):
        return t1
    else:
        return 100

DRAG = 0.8

class AABB:
    def __init__(self, pos, size):
        self.pos = pos
        self.size = size
        self.vel = [0.0,0.0]
        self.force = [0.0,0.0]
    def draw(self, screen):
        pygame.draw.rect(screen, (100,100,100), (self.min()[0], self.min()[1], self.size[0], self.size[1]))
    def addForce(self, dv):
        self.force[0] += dv[0]
        self.force[1] += dv[1]
    def setXForce(self, vx):
        self.force[0] = vx
    def setYForce(self, vy):
        self.force[1] = vy
    def max(self):
        return [self.pos[0] + self.size[0]/2, self.pos[1] + self.size[1]/2]
    def min(self):
        return [self.pos[0] - self.size[0]/2, self.pos[1] - self.size[1]/2]    
    def update(self, aabs, dt, screen):
        lastPos = self.pos[:]
        self.vel[0] += self.force[0]*dt
        self.vel[1] += self.force[1]*dt
        self.pos[0] += self.vel[0]*dt
        self.pos[1] += self.vel[1]*dt
        self.vel[0] *= DRAG
        self.vel[1] *= DRAG

        for b in aabs:
            if self is not b and (abs(self.vel[0]) > 0 or abs(self.vel[1]) > 0):
                thisMin = self.min()
                thisMax = self.max()
                otherMin = b.min()
                otherMax = b.max()
                if (thisMax[0] > otherMin[0] and thisMin[0] < otherMax[0] and
                    thisMax[1] > otherMin[1] and thisMin[1] < otherMax[1]):
                    print("COLLISION!")

                    pC = [0.0, 0.0]
                    pH = [0.0, 0.0]
                    pV = [0.0, 0.0]
                    opC = [0.0, 0.0]
                    opH = [0.0, 0.0]
                    opV = [0.0, 0.0]
                    if thisMin[0] <= otherMin[0]: # and self.vel[0] > 0:
                        pC[0] = thisMax[0]
                        pH[0] = thisMin[0]
                        pV[0] = thisMax[0]
                        opC[0] = otherMin[0]
                        opH[0] = otherMax[0]
                        opV[0] = otherMin[0]
                    elif thisMax[0] >= otherMax[0]: # and self.vel[0] < 0:
                        pC[0] = thisMin[0]
                        pH[0] = thisMax[0]
                        pV[0] = thisMin[0]
                        opC[0] = otherMax[0]
                        opH[0] = otherMin[0]
                        opV[0] = otherMax[0]
                        
                    if thisMin[1] <= otherMin[1]: # and self.vel[1] > 0:
                        pC[1] = thisMax[1]
                        pH[1] = thisMax[1]
                        pV[1] = thisMin[1]
                        opC[1] = otherMin[1]
                        opH[1] = otherMin[1]
                        opV[1] = otherMax[1]
                    elif thisMax[1] >= otherMax[1]: # and self.vel[1] < 0:
                        pC[1] = thisMin[1]
                        pH[1] = thisMin[1]
                        pV[1] = thisMax[1]
                        opC[1] = otherMax[1]
                        opH[1] = otherMax[1]
                        opV[1] = otherMin[1]

                    print("PC: (%f, %f), PH: (%f, %f), PV: (%f, %f)" % (pC[0], pC[1], pH[0], pH[1], pV[0], pV[1]))
                    print("OPC: (%f, %f), OPH: (%f, %f), OPV: (%f, %f)" % (opC[0], opC[1], opH[0], opH[1], opV[0], opV[1]))
                    
                    tCV = intersect([pC, self.vel], [opC, opV], screen)
                    tCH = intersect([pC, self.vel], [opC, opH], screen)
                    tVV = intersect([pV, self.vel], [opC, opV], screen)
                    tVH = intersect([pV, self.vel], [opC, opH], screen)
                    tHV = intersect([pH, self.vel], [opC, opV], screen)
                    tHH = intersect([pH, self.vel], [opC, opH], screen)

                    if min([tCH, tVH, tHH]) < min([tCV, tVV, tHV]) and (self.vel[1] < 0) != (opC[1] - pC[1] < 0):
                        self.pos[1] += opC[1] - pC[1]
                        self.vel[1] = 0.0
                    elif min([tCH, tVH, tHH]) > min([tCV, tVV, tHV]) and (self.vel[0] < 0) != (opC[0] - pC[0] < 0):
                        self.pos[0] += opC[0] - pC[0]
                        self.vel[0] = 0.0

                    print("TCV: %f, TCH: %f  |  TVV: %f, TVH: %f  |  THV: %f, THH: %f" % (tCV, tCH, tVV, tVH, tHV, tHH))

if __name__ == "__main__":
    pygame.init()
    screen = pygame.display.set_mode((800,600))

    aabbs = [AABB([10.0,10.0], [100.0,100.0]), AABB([300.0,300.0], [100.0,100.0])]
    
    try:
        running = True
        while running:
            for e in pygame.event.get():
                if e.type == pygame.QUIT:
                    running = False
                if e.type == pygame.KEYDOWN:
                    if e.key == pygame.K_w:
                        aabbs[0].addForce((0.0,-40.0))
                    elif e.key == pygame.K_s:
                        aabbs[0].addForce((0.0,40.0))
                    elif e.key == pygame.K_a:
                        aabbs[0].addForce((-40.0,0.0))
                    elif e.key == pygame.K_d:
                        aabbs[0].addForce((40.0,0.0))
                if e.type == pygame.KEYUP:
                    if e.key == pygame.K_w:
                        aabbs[0].setYForce(0.0)
                    elif e.key == pygame.K_s:
                        aabbs[0].setYForce(0.0)
                    elif e.key == pygame.K_a:
                        aabbs[0].setXForce(0.0)
                    elif e.key == pygame.K_d:
                        aabbs[0].setXForce(0.0)
            screen.fill((10,10,10))
            for b in aabbs:
                b.draw(screen)
            for b in aabbs:
                b.update(aabbs, 0.016, screen)
            pygame.display.update()
    except KeyboardInterrupt:
        print("Ctrl-C...")
    finally:
        print("Quitting...")
        pygame.quit()
