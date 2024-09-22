/**
 * Manage Node Card on devices page
 */
class NodeCard {

    static instances = []

    static getInstance(id) {
        return NodeCard.instances.find(i => i.data.did === id)
    }

    static parseEvent(data) {
        const [event, did, prop, value] = data.split('_')
        const node = NodeCard.getInstance(did)
        if (!node) return
        switch (event) {
            case 'PROPCHANGED':
                node.updateProp(prop, value)
                break
            case 'ONLINE':
                node.markOnline(true)
                break
            case 'OFFLINE':
                node.markOnline(false)
                break
        }
    }

    constructor(data) {
        if (!data) return
        if (NodeCard.getInstance(data.did))
            throw new Error(`NodeCard [${data.did}] already exists`)
        NodeCard.instances.push(this)
        this.data = data
        this.el = null
        this.isOnline = this.data.online === 'true'
        this.init()
        this.markOnline(this.isOnline)
    }

    init() {
        {
            const { name, did, id, props_name } = this.data
            const props = props_name?.split(',').map(p => ({ id: p, ...this.parseProp(this.data[p]) }))

            const nodeHtml = document.createElement('div')
            nodeHtml.className = 'col-12 col-md-6 col-lg-4 col-xxl-3 col-node my-3'
            nodeHtml.setAttribute('did', did)
            nodeHtml.innerHTML = `<div class="card node-card">
                <div class="card-header d-flex align-items-center fs-4 mb-3">
                    <span>${name}</span>
                    <span class="text-muted fs-6 ms-2">${id}</span>
                    <div class="ms-auto pointer ps-4 pe-2" data-bs-toggle="dropdown"><i class="fas fa-ellipsis-v"></i></div>
                    <div class="dropdown-menu">
                        <a class="dropdown-item" action="deviceDetail" href="#"><i class="fal fa-info-circle me-2"></i><span>Detail</span></a>
                        <a class="dropdown-item" action="syncProps" href="#"><i class="fal fa-cloud-download me-2"></i><span>Sync</span></a>
                        <a class="dropdown-item" action="restartDevice" href="#"><i class="fal fa-sync me-2"></i><span>Restart</span></a>
                        <a class="dropdown-item text-danger" action="unpairDevice" href="#"><i class="far fa-unlink me-2"></i><span>Unpair</span></a>
                    </div>
                </div>
                <div class="card-body-container">
                    <div class="card-body fs-5">${props?.map(this.propHTML).join('')}</div>
                </div>
                <div class="card-footer text-center">
                    <i class="text-danger opacity-50 fad fa-circle"></i>
                    <span class="ms-1 text-muted">offline</span>
                </div>
            </div>`
            $main.appendChild(nodeHtml)
            this.el = nodeHtml
        }
    }

    parseProp(pval) {
        let val = ''
        let type = ''
        const i = pval.indexOf(',')
        if (i > -1) {
            val = pval.substring(0, i)
            const jstr = pval.substring(i + 1)
            let json = null
            try { json = JSON.parse(jstr) } catch (error) { }
            if (json) return { v: val, ...json }
            return { v: pval, type: 'string' }
        }
        if (pval === 'true' || pval === 'false') type = 'bool'
        else if (!isNaN(pval)) type = 'number'
        else type = 'string'
        return { v: pval, type }
    }
    propHTML(prop) {
        const pname = () => `<span>${prop.name || prop.id}${prop.name ? (" <span class=\"text-muted\">" + prop.id + "</span>") : ""}</span>`
        const pbool = () => `<div class="ms-auto form-check form-switch"><input class="form-check-input" type="checkbox" role="switch" action="setProp" prop="${prop.id}" ${prop.v === 'true' ? "checked" : ""}></div>`
        const pinput = () => `<input class="ms-auto form-control text-end" type="text" action="setProp" prop="${prop.id}" value="${prop.v}" ${prop.type == 'number' ? 'pattern="\d*"' : ''}>`
        let html = '<div class="d-flex align-items-center mb-3">' + pname()
        switch (prop.type) {
            case 'bool': html += pbool(); break;
            case 'number': case 'string': html += pinput(); break;
        }
        html += '</div>'
        return html
    }

    markOnline(online = true) {
        this.isOnline = online
        if (online) {
            this.el.querySelector('.node-card').classList.remove('offline')
            this.el.querySelector('[action="syncProps"]').classList.remove('disabled')
            this.el.querySelector('[action="restartDevice"]').classList.remove('disabled')
        } else {
            this.el.querySelector('.node-card').classList.add('offline')
            this.el.querySelector('[action="syncProps"]').classList.add('disabled')
            this.el.querySelector('[action="restartDevice"]').classList.add('disabled')
        }
    }

    updateProp(name, value) {
        const prop = this.el.querySelector(`[prop="${name}"]`)
        if (!prop) return
        if (prop.type === 'checkbox') {
            prop.checked = value === 'true'
        } else {
            prop.value = value
        }
        this.data[name] = value
    }

    revertProp(name) {
        const prop = this.el.querySelector(`[prop="${name}"]`)
        if (!prop) return
        if (prop.type === 'checkbox') {
            prop.checked = this.data[name] === 'true'
        } else {
            prop.value = this.data[name]
        }
    }

    setProp(prop, value) {
        const { did } = this.data
        GET(baseURI + '/command?' + json2query({ id: did, action: 'set_prop', params: `${prop},${value}` }))
            .then(data => {
                console.log(data)
                if (data.success === true) {
                    notify('success', `[${did}/${prop}] changed to [${value}]`)
                    return
                }
                notify('failure', `Change [${did}/${prop}] failed: ${data.message}`)
                this.revertProp(prop)
            })
            .catch(error => {
                notify('failure', `Change [${did}/${prop}] failed: ${error.message}`)
                this.revertProp(prop)
            })
    }

    showDetail() {
        document.querySelector('#detailModal tbody').innerHTML = ''
        let html = Object.keys(this.data).map(key => `<tr><td>${key}</td><td>${this.data[key]}</td></tr>`).join('')
        document.querySelector('#detailModal tbody').innerHTML = html
        new bootstrap.Modal(document.getElementById('detailModal')).show()
    }
}


const setProp = (did, prop, value)  => {
    NodeCard.getInstance(did)?.setProp(prop, value)
}