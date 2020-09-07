#include "TinyEngine.h"
#include "include/helpers/image.h"
#include "include/helpers/color.h"
#include "include/helpers/helper.h"

struct Branch {

	int ID = 0;         //For Leaf Hashing
	bool leaf = true;

	Branch *A, *B, *P;  //Child A, B and Parent

	//Parameters
	float ratio, spread, splitsize;
	int depth = 0;

	//Constructors
	Branch(float r, float s, float ss) :
		ratio{ r },
		spread{ s },
		splitsize{ ss }
	{};

	Branch(Branch* b, bool root) :
		ratio{ b->ratio },
		spread{ b->spread },
		splitsize{ b->splitsize }
	{
		if (root) return;
		depth = b->depth + 1;
		P = b;  //Set Parent
	};

	~Branch() {
		if (leaf) return;
		delete(A);
		delete(B);
	}

	//Size / Direction Data
	glm::vec3 dir = glm::vec3(0.0, 1.0, 0.0);
	float length = 0.0, radius = 0.0, area = 0.1;

	void grow(double _size);
	void split();

	//Compute Direction to Highest Local Leaf Density
	glm::vec3 leafdensity(int searchdepth);
};

void Branch::grow(double feed) {

	radius = sqrt(area / PI);   //Current Radius

	if (leaf) {
		length += cbrt(feed);   //Grow in Length
		feed -= length * area;  //Reduce Feed
		area += feed / length;    //Grow In Area

		//Split Condition
		if (length > splitsize * exp(-splitdecay * depth))
			split();  //Split Behavior

		return;
	}

	double pass = passratio;

	if (conservearea)  //Feedback Control for Area Conservation
		pass = (A->area + B->area) / (A->area + B->area + area);

	area += pass * feed / length;   //Grow in Girth
	feed *= (1.0 - pass);         //Reduce Feed

	if (feed < 1E-5) return;         //Prevent Over-Branching

	A->grow(feed*ratio);            //Grow Children
	B->grow(feed*(1.0 - ratio));
}

void Branch::split() {

	leaf = false;

	//Add Child Branches
	A = new Branch(this, false);
	B = new Branch(this, false);
	A->ID = 2 * ID + 0; //Every Leaf ID is Unique (because binary!)
	B->ID = 2 * ID + 1;

	/*  Ideal Growth Direction:
		  Perpendicular to direction with highest leaf density! */

	glm::vec3 D = leafdensity(localdepth);            //Direction of Highest Density
	glm::vec3 N = glm::normalize(glm::cross(dir, D)); //Normal Vector
	glm::vec3 M = -1.0f*N;                            //Reflection

	float flip = (rand() % 2) ? 1.0 : -1.0; //Random Direction Flip
	A->dir = glm::normalize(glm::mix(flip*spread*N, dir, ratio));
	B->dir = glm::normalize(glm::mix(flip*spread*M, dir, 1.0 - ratio));

}

glm::vec3 Branch::leafdensity(int searchdepth) {

	//Random Vector! (for noise)
	glm::vec3 r = glm::vec3(rand() % 100, rand() % 100, rand() % 100) / glm::vec3(100) - glm::vec3(0.5);

	if (depth == 0) return r;

	/*
	  General Idea: Branches grow away from areas with a high leaf density!

	  Therefore, if we can compute a vector that points towards the area with
	  the locally highest leaf density, we can use that to compute our normal
	  for branching.

	  Locally high density is determined by descending the tree to some maximum
	  search depth (finding an ancestor node), and computing some leaf-density
	  metric over the descendant node leaves. This is implemented recursively.

	  Metric 1: Uniform Weights in Space.
		Problem: Causes strange spiral artifacts at high-search depths, because it
		  computes the average leaf position of the entire tree. Therefore,
		  the tree grows in a strange way, away from the center.

	  Metric 2: Distance weighted average position (i.e. relative vector)
		Problem: This causes strange cone artifacts when using a sufficiently large
		  search depth. This is also more expensive to compute, and wonky because
		  we compute the distance-weighted average distance (what?? exactly).

	  Since both metrics give similar results at a small search-depth (e.g. 2),
	  meaning we only search locally, I will use the one that is simpler to compute.
	  That is Method 1.

	  I did throw in a weighting by the branch ratio though, just because I can.
	  That means that the tree should tend to grow away from branches with the most
	  growth potential.

	*/

	Branch* C = this;                                 //Ancestor node
	glm::vec3 rel = glm::vec3(0);                     //Relative position to start node
	while (C->depth > 0 && searchdepth-- >= 0) {        //Descend tree
		rel += C->length*C->dir;                        //Add relative position
		C = C->P;                                       //Move to parent
	}

	std::function<glm::vec3(Branch*)> leafaverage = [&](Branch* b)->glm::vec3 {
		if (b->leaf) return b->length*b->dir;
		return b->length*b->dir + ratio * leafaverage(b->A) + (1.0f - ratio)*leafaverage(b->B);
	};

	//Average relative to ancestor, shifted by rel ( + Noise )
	return directedness * glm::normalize(leafaverage(C) - rel) + (1.0f - directedness)*r;
}

Branch* root;

// Model Constructing Function for Tree
std::function<void(Model*)> _construct = [&](Model* h) {

	//Basically Add Lines for the Tree!
	std::function<void(Branch*, glm::vec3)> addBranch = [&](Branch* b, glm::vec3 p) {

		glm::vec3 start = p;
		glm::vec3 end = p + glm::vec3(b->length*treescale[0])*b->dir;

		//Get Some Normal Vector
		glm::vec3 x = glm::normalize(b->dir + glm::vec3(1.0, 1.0, 1.0));
		glm::vec4 n = glm::vec4(glm::normalize(glm::cross(b->dir, x)), 1.0);

		//Add the Correct Number of Indices
		glm::mat4 r = glm::rotate(glm::mat4(1.0), PI / ringsize, b->dir);

		//Index Buffer
		int _b = h->positions.size() / 3;

		//GL TRIANGLES
		for (int i = 0; i < ringsize; i++) {
			//Bottom Triangle
			h->indices.push_back(_b + i * 2 + 0);
			h->indices.push_back(_b + (i * 2 + 2) % (2 * ringsize));
			h->indices.push_back(_b + i * 2 + 1);
			//Upper Triangle
			h->indices.push_back(_b + (i * 2 + 2) % (2 * ringsize));
			h->indices.push_back(_b + (i * 2 + 3) % (2 * ringsize));
			h->indices.push_back(_b + i * 2 + 1);
		}

		for (int i = 0; i < ringsize; i++) {

			h->positions.push_back(start.x + b->radius*treescale[1] * n.x);
			h->positions.push_back(start.y + b->radius*treescale[1] * n.y);
			h->positions.push_back(start.z + b->radius*treescale[1] * n.z);
			h->normals.push_back(n.x);
			h->normals.push_back(n.y);
			h->normals.push_back(n.z);
			n = r * n;

			h->positions.push_back(end.x + taper * b->radius*treescale[1] * n.x);
			h->positions.push_back(end.y + taper * b->radius*treescale[1] * n.y);
			h->positions.push_back(end.z + taper * b->radius*treescale[1] * n.z);
			h->normals.push_back(n.x);
			h->normals.push_back(n.y);
			h->normals.push_back(n.z);
			n = r * n;

		}

		//No Leaves
		if (b->leaf) return;

		addBranch(b->A, end);
		addBranch(b->B, end);
	};

	//Recursive add Branches
	addBranch(root, glm::vec3(0.0));
};

//Construct Leaf Particle System from Tree Data
std::function<void(std::vector<glm::mat4>&, bool)> addLeaves = [&](std::vector<glm::mat4>& p, bool face) {
	p.clear();

	//Explore the Tree and Add Leaves!
	std::function<void(Branch*, glm::vec3)> addLeaf = [&](Branch* b, glm::vec3 pos) {

		if (b->leaf) {

			if (b->depth < leafmindepth) return;

			for (int i = 0; i < leafcount; i++) {
				//Hashed Random Displace
				glm::vec3 d = glm::vec3(hashrand(b->ID + i), hashrand(b->ID + i + leafcount), hashrand(b->ID + i + 2 * leafcount)) - glm::vec3(0.5);
				d = d * glm::vec3(leafspread[0], leafspread[1], leafspread[2]);

				//Rotate Towards Camera (or not) and Scale
				glm::mat4 model = glm::translate(glm::mat4(1.0), pos + d);

				if (face) model = glm::rotate(model, glm::radians(45.0f - rotation), glm::vec3(0.0, 1.0, 0.0));
				else model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0, 1.0, 0.0));

				p.push_back(glm::scale(model, glm::vec3(leafsize)));

			}

			return;
		}

		//Otherwise get other leaves!
		glm::vec3 end = pos + glm::vec3(b->length*treescale[0])*b->dir;
		addLeaf(b->A, end);
		addLeaf(b->B, end);
	};

	addLeaf(root, glm::vec3(0.0));
};

//Parameters
#define PI 3.14159265f

const int WIDTH = 1200;
const int HEIGHT = 800;

float zoom = 0.5;
float zoomInc = 0.99;
float rotation = 0.0f;
glm::vec3 cameraPos = glm::vec3(50, 200, 50);
glm::vec3 lookPos = glm::vec3(0, 180, 0);
glm::mat4 camera = glm::lookAt(cameraPos, lookPos, glm::vec3(0, 1, 0));
glm::mat4 projection;

bool paused = false;
bool autorotate = true;
bool drawwire = false;
bool drawtree = true;
bool drawleaf = true;

float leafcolor[3] = { 0.82, 0.13, 0.23 };
float treecolor[3] = { 1.00, 1.00, 1.00 };
float wirecolor[3] = { 0.00, 0.00, 0.00 };
float backcolor[3] = { 0.80, 0.80, 0.80 };
float lightcolor[3] = { 1.00, 1.00, 1.00 };
float leafopacity = 0.9;
int leafmindepth = 8;
float treeopacity = 1.0;
float treescale[2] = { 15.0f, 5.0f };

int ringsize = 12;
int leafcount = 10;
float leafsize = 5.0;
float taper = 0.6;
float leafspread[3] = { 50.0, 50.0, 50.0 };

float growthrate = 1.0;
float passratio = 0.3;
float splitdecay = 1E-2;
float directedness = 0.5;
int localdepth = 2;
bool conservearea = true;

bool drawshadow = true;
bool selfshadow = true;
bool leafshadow = true;
glm::vec3 lightpos = glm::vec3(50);
glm::mat4 bias = glm::mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);
glm::mat4 lproj = glm::ortho(-300.0f, 300.0f, -300.0f, 400.0f, -200.0f, 800.0f);
glm::mat4 lview = glm::lookAt(lightpos, glm::vec3(0), glm::vec3(0, 1, 0));

#include "tree.h"

void setup() {
	projection = glm::ortho(-(float)Tiny::view.WIDTH*zoom, (float)Tiny::view.WIDTH*zoom, -(float)Tiny::view.HEIGHT*zoom, (float)Tiny::view.HEIGHT*zoom, -500.0f, 800.0f);
	srand(time(NULL));
	root = new Branch({ 0.6, 0.45, 2.5 }); //Create Root
}

// Event Handler
std::function<void()> eventHandler = [&]() {

	if (Tiny::event.scroll.posy) {
		zoom /= zoomInc;
		projection = glm::ortho(-(float)Tiny::view.WIDTH*zoom, (float)Tiny::view.WIDTH*zoom, -(float)Tiny::view.HEIGHT*zoom, (float)Tiny::view.HEIGHT*zoom, -500.0f, 800.0f);
	}
	if (Tiny::event.scroll.negy) {
		zoom *= zoomInc;
		projection = glm::ortho(-(float)Tiny::view.WIDTH*zoom, (float)Tiny::view.WIDTH*zoom, -(float)Tiny::view.HEIGHT*zoom, (float)Tiny::view.HEIGHT*zoom, -500.0f, 800.0f);
	}
	if (Tiny::event.scroll.posx) {
		rotation += 1.5f;
		if (rotation < 0.0) rotation = 360.0 + rotation;
		else if (rotation > 360.0) rotation = rotation - 360.0;
		camera = glm::rotate(camera, glm::radians(1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	if (Tiny::event.scroll.negx) {
		rotation -= 1.5f;
		if (rotation < 0.0) rotation = 360.0 + rotation;
		else if (rotation > 360.0) rotation = rotation - 360.0;
		camera = glm::rotate(camera, glm::radians(-1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	//Pause Toggle
	if (!Tiny::event.press.empty()) {
		if (Tiny::event.press.back() == SDLK_p)
			paused = !paused;
		else if (Tiny::event.press.back() == SDLK_a)
			autorotate = !autorotate;

		//Regrow
		else if (Tiny::event.press.back() == SDLK_r) {
			Branch* newroot = new Branch(root, true);
			delete(root);
			root = newroot;
		}
	}

};

//Interface Function
Handle interfaceFunc = [&]() {
	//Window Size
	ImGui::SetNextWindowSize(ImVec2(360, 400), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);

	//Open Window
	ImGui::Begin("Tree Controller", NULL, ImGuiWindowFlags_None);
	if (ImGui::BeginTabBar("Tab Bar", ImGuiTabBarFlags_None)) {
		if (ImGui::BeginTabItem("Info")) {

			ImGui::Checkbox("Pause [P]", &paused);
			ImGui::Checkbox("Auto-Rotate [A]", &autorotate);

			ImGui::ColorEdit3("Background", backcolor);
			//        ImGui::ColorEdit3("Light Color", lightcolor);

			ImGui::Text("Made by Nicholas McDonald");
			ImGui::EndTabItem();

		}

		if (ImGui::BeginTabItem("Growth")) {

			if (ImGui::Button("Re-Grow [R]")) {
				Branch* newroot = new Branch(root, true);
				delete(root);
				root = newroot;
			}

			ImGui::Text("Growth Behavior");
			ImGui::DragFloat("Growth Rate", &growthrate, 0.01f, 0.0f, 5.0f);
			ImGui::Checkbox("Conserve Crossectional Area", &conservearea);
			if (!conservearea)
				ImGui::DragFloat("Pass Ratio", &passratio, 0.01f, 0.0f, 1.0f);

			ImGui::Text("Split Behavior");
			ImGui::DragFloat("Ratio", &root->ratio, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Size", &root->splitsize, 0.1f, 0.1f, 5.0f);
			ImGui::DragFloat("Decay", &splitdecay, 0.001f, 0.0f, 1.0f);

			ImGui::Text("Growth Direction");
			ImGui::DragFloat("Spread", &root->spread, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("Directedness", &directedness, 0.01f, 0.0f, 1.0f);
			ImGui::DragInt("Local Depth", &localdepth, 1, 0, 15);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Leaf")) {
			ImGui::ColorEdit3("Color", leafcolor);
			ImGui::DragFloat("Opacity", &leafopacity, 0.01f, 0.0f, 1.0f);
			ImGui::DragInt("Count", &leafcount, 1, 0, 30);
			ImGui::DragInt("Minimum Depth", &leafmindepth, 1, 0, 15);
			ImGui::DragFloat3("Spread", leafspread, 0.1f, 0.0f, 250.0f);
			ImGui::DragFloat("Size", &leafsize, 0.1f, 0.0f, 25.0f);
			ImGui::Checkbox("Draw", &drawleaf); ImGui::SameLine();
			ImGui::Checkbox("Shade", &leafshadow); ImGui::SameLine();
			ImGui::Checkbox("Self-Shade", &selfshadow);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Tree")) {
			ImGui::ColorEdit3("Fill", treecolor);
			ImGui::ColorEdit3("Wire", wirecolor);
			ImGui::DragFloat("Opacity", &treeopacity, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat2("Scale", treescale, 0.01f, 0.1f, 50.0f);
			ImGui::DragFloat("Taper", &taper, 0.01f, 0.0f, 1.0f);

			ImGui::Checkbox("Draw", &drawtree); ImGui::SameLine();
			ImGui::Checkbox("Wire", &drawwire); ImGui::SameLine();
			ImGui::Checkbox("Shade", &drawshadow);

			ImGui::DragInt("Mesh", &ringsize, 1, 3, 12);

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
};

std::function<void(Model* m)> construct_floor = [&](Model* h) {

	float floor[24] = {
	  -1.0, 0.0, -1.0,
	  -1.0, 0.0,  1.0,
	   1.0, 0.0, -1.0,
	   1.0, 0.0,  1.0,
	};

	for (int i = 0; i < 12; i++)
		h->positions.push_back(floor[i]);

	h->indices.push_back(0);
	h->indices.push_back(1);
	h->indices.push_back(2);

	h->indices.push_back(1);
	h->indices.push_back(3);
	h->indices.push_back(2);

	glm::vec3 floorcolor = glm::vec3(0.65, 0.5, 0.3);

	for (int i = 0; i < 4; i++) {
		h->normals.push_back(0.0);
		h->normals.push_back(1.0);
		h->normals.push_back(0.0);

		h->colors.push_back(floorcolor.x);
		h->colors.push_back(floorcolor.y);
		h->colors.push_back(floorcolor.z);
		h->colors.push_back(1.0);
	}
};

int main(int argc, char* args[]) {

	Tiny::view.lineWidth = 1.0f;

	Tiny::window("Procedural Tree", WIDTH, HEIGHT);
	Tiny::event.handler = eventHandler;
	Tiny::view.interface = interfaceFunc;

	setup();																				//Prepare Model Stuff

	Model treemesh(_construct);											//Construct a Mesh

	Square3D flat;																	//Geometry for Particle System

	std::vector<glm::mat4> leaves;
	addLeaves(leaves, true);												//Generate the model matrices

	Instance particle(&flat);												//Make Particle System
	particle.addBuffer(leaves);											//Add Matrices

	Texture tex(image::load("leaf.png"));

	Shader particleShader({ "shader/particle.vs", "shader/particle.fs" }, { "in_Quad", "in_Tex", "in_Model" });
	Shader defaultShader({ "shader/default.vs", "shader/default.fs" }, { "in_Position", "in_Normal" });
	Shader depth({ "shader/depth.vs", "shader/depth.fs" }, { "in_Position" });
	Shader particledepth({ "shader/particledepth.vs", "shader/particledepth.fs" }, { "in_Quad", "in_Tex", "in_Model" });
	Billboard shadow(1600, 1600, false); 						//No Color Buffer

	Model floor(construct_floor);
	floor.move(glm::vec3(0), 0, glm::vec3(1000));	//So we can cast shadows

	Tiny::view.pipeline = [&]() {	//Setup Drawing Pipeline

		shadow.target();
		if (drawshadow) {
			depth.use();
			depth.uniform("dvp", lproj*lview);
			defaultShader.uniform("model", treemesh.model);
			treemesh.render(GL_TRIANGLES);
		}
		if (leafshadow) {
			particledepth.use();
			particledepth.uniform("dvp", lproj*lview);
			particledepth.texture("spriteTexture", tex);
			addLeaves(leaves, false);						//Generate the model matrices
			particle.updateBuffer(leaves, 0);

			particle.render(GL_TRIANGLE_STRIP); 		//Render Particle System
		}

		//Prepare Render Target
		Tiny::view.target(glm::vec3(backcolor[0], backcolor[1], backcolor[2]));

		if (drawwire || drawtree) {
			defaultShader.use();
			defaultShader.uniform("model", treemesh.model);
			defaultShader.uniform("projectionCamera", projection*camera);
			defaultShader.uniform("lightcolor", lightcolor);
			defaultShader.uniform("lookDir", lookPos - cameraPos);
			defaultShader.uniform("lightDir", lightpos);

			defaultShader.uniform("drawshadow", drawshadow);
			if (drawshadow) {
				defaultShader.uniform("dbvp", bias*lproj*lview);
				defaultShader.texture("shadowMap", shadow.depth);
				defaultShader.uniform("light", lightpos);
			}

			defaultShader.uniform("drawfloor", true);
			defaultShader.uniform("drawcolor", glm::vec4(backcolor[0], backcolor[1], backcolor[2], 1));
			defaultShader.uniform("model", floor.model);
			floor.render();
			defaultShader.uniform("drawfloor", false);

			defaultShader.uniform("model", treemesh.model);

			if (drawtree) {
				defaultShader.uniform("drawcolor", glm::vec4(treecolor[0], treecolor[1], treecolor[2], treeopacity));
				defaultShader.uniform("wireframe", false);
				treemesh.render(GL_TRIANGLES);
			}

			if (drawwire) {
				defaultShader.uniform("drawcolor", glm::vec4(wirecolor[0], wirecolor[1], wirecolor[2], 1.0));
				defaultShader.uniform("wireframe", true);
				treemesh.render(GL_LINES);
			}
		}

		if (drawleaf) {
			particleShader.use();
			particleShader.texture("spriteTexture", tex);
			particleShader.uniform("projectionCamera", projection*camera);
			particleShader.uniform("leafcolor", glm::vec4(leafcolor[0], leafcolor[1], leafcolor[2], leafopacity));
			particleShader.uniform("lightcolor", lightcolor);

			particleShader.uniform("selfshadow", selfshadow);
			if (selfshadow) {
				particleShader.uniform("dbvp", bias*lproj*lview);
				particleShader.texture("shadowMap", shadow.depth);
				particleShader.uniform("light", lightpos);
			}

			particleShader.uniform("lookDir", lookPos - cameraPos);
			addLeaves(leaves, true);
			particle.updateBuffer(leaves, 0);
			particle.render(GL_TRIANGLE_STRIP); //Render Particle System
		}
	};

	//Loop over Stuff
	Tiny::loop([&]() { /* ... */

		if (autorotate) {
			camera = glm::rotate(camera, glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
			rotation += 0.5f;
		}

		if (!paused)
			root->grow(growthrate);

		//Update Rendering Structures
		treemesh.construct(_construct);
		particle.updateBuffer(leaves, 0);

		});

	//Get rid of this thing!
	delete root;

	Tiny::quit();

	return 0;
}